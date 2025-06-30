#include <cstdlib>
#include <format>
#include <initializer_list>
#include <list>
#include <memory>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// === --- Macros and Configurations --------------------------------------- ===

// The prefix and suffix to emit for vm bytecode.
#define VM_BYTE_CODE_PREFIX \
        "        CHECK_AND_JUMP( vm_program_push_op( program, "
#define VM_BYTE_CODE_SUFFIX " ) );\n"

// Panic in case things go wrong.
#define PANIC( fmt, ... )                            \
        do {                                         \
                std::print( fmt "\n", __VA_ARGS__ ); \
                std::abort( );                       \
        } while ( 0 )

// === --- The Value and Ops ----------------------------------------------- ===

namespace aix {

/* Value is the base class for all inputs and outputs. */
struct Value {
        virtual ~Value( ) = default;

        /* Returns the debug JSON string. */
        virtual auto getDebugJson( ) -> std::string = 0;

        /* Emit the VM bytecode assembly. */
        virtual auto emit( ) -> void = 0;
};

/* A Named Input represents a placeholder external input. */
struct NamedInput : public Value {
      public:
        NamedInput( std::string_view name ) : name_( name ) {}

        auto getDebugJson( ) -> std::string override
        {
                return std::format( R"({{ "type": "{}", "name": "{}" }})",
                                    getInputJsonType( ), name_ );
        };

        auto emit( ) -> void override
        {
                std::print( VM_BYTE_CODE_PREFIX
                            "OP_LOAD_{}, {}" VM_BYTE_CODE_SUFFIX,
                            getInputOpType( ), getInputStringToEmitVMCode( ) );
        }

      protected:
        virtual auto getInputJsonType( ) const -> std::string_view = 0;
        virtual auto getInputOpType( ) const -> std::string_view   = 0;

        auto getParamName( ) const -> const std::string & { return name_; }
        auto getInputStringToEmitVMCode( ) const -> const std::string
        {
                return std::format( "model->{0}.name, model->{0}.weight",
                                    name_ );
        }

      private:
        std::string name_;
};

/* A Param represents a short live input or parameter. */
struct Param : public NamedInput {
      public:
        Param( std::string_view name ) : NamedInput( name ) {}

      protected:
        auto getInputJsonType( ) const -> std::string_view override
        {
                return "param";
        }
        auto getInputOpType( ) const -> std::string_view override
        {
                return "PARAM";
        }
};

/* A Weight represents a long live model weight. */
struct Weight : public NamedInput {
      public:
        Weight( std::string_view name ) : NamedInput( name ) {}

      protected:
        auto getInputJsonType( ) const -> std::string_view override
        {
                return "weight";
        }
        auto getInputOpType( ) const -> std::string_view override
        {
                return "WEIGHT";
        }
};

enum class OpKind {
        Gatter,
        Dense,
        AssertEq,  // A special Op to assert two value's equalness.
};

template <int N>
struct Op : public Value {
      public:
        Op( OpKind kind, std::initializer_list<Value *> operands )
            : kind_( kind )
        {
                operands_.reserve( N );
                operands_.insert( operands_.end( ), operands );

#ifndef NDEBUG
                /* Sanity check no operand's input is assert_eq. */
                for ( auto &base_op : operands ) {
                        auto *op = dynamic_cast<Op<2> *>( base_op );
                        if ( op != nullptr && op->kind_ == OpKind::AssertEq ) {
                                PANIC(
                                    "assert_eq cannot be used as Op input. op: "
                                    "{}",
                                    op->getDebugJson( ) );
                        }
                }
#endif
        }

        auto getDebugJson( ) -> std::string override
        {
                switch ( kind_ ) {
                case OpKind::Gatter:
                        return std::format(
                            R"({{ "type": "op", "kind": "gatter", "input": {}, "table": {} }})",
                            operands_[0]->getDebugJson( ),
                            operands_[1]->getDebugJson( ) );
                case OpKind::Dense:
                        return std::format(
                            R"({{ "type": "op", "kind": "dense", "input": {}, "weight": {} }})",
                            operands_[0]->getDebugJson( ),
                            operands_[1]->getDebugJson( ) );
                case OpKind::AssertEq:
                        return std::format(
                            R"({{ "type": "op", "kind": "assert_eq", "expected": {}, "got": {} }})",
                            operands_[0]->getDebugJson( ),
                            operands_[1]->getDebugJson( ) );
                default:
                        return "(unknown op)";
                }
        }

        auto emit( ) -> void override
        {
                for ( auto &operand :
                      std::ranges::views::reverse( operands_ ) ) {
                        operand->emit( );
                }
                switch ( kind_ ) {
                case OpKind::Gatter:
                        std::print( VM_BYTE_CODE_PREFIX
                                    "OP_GATTER" VM_BYTE_CODE_SUFFIX );
                        break;
                case OpKind::Dense:
                        std::print( VM_BYTE_CODE_PREFIX
                                    "OP_MATMUL" VM_BYTE_CODE_SUFFIX );
                        break;
                case OpKind::AssertEq:
                        std::print( VM_BYTE_CODE_PREFIX
                                    "OP_ASSERT_EQ" VM_BYTE_CODE_SUFFIX );
                        break;
                default:
                        PANIC( "unsupported op: {}", int( kind_ ) );
                }
        }

      private:
        OpKind               kind_;
        std::vector<Value *> operands_;
};

class Program {
      private:
        std::list<std::unique_ptr<Value>>                       ops_;
        std::unordered_map<std::string, std::unique_ptr<Value>> params_;
        std::unordered_map<std::string, std::unique_ptr<Value>> weights_;

      public:
        /* Registers a new parameter. */
        auto registerParam( std::string_view name ) -> Value *
        {
                auto key = std::string{ name };
                if ( params_.contains( key ) ) {
                        PANIC( "param key {} already registed.", key );
                }
                auto v   = std::make_unique<Param>( key );
                auto ptr = v.get( );

                params_.emplace( std::move( key ), std::move( v ) );
                return ptr;
        }

        /* Registers a new model weight. */
        auto registerWeight( std::string_view name ) -> Value *
        {
                auto key = std::string{ name };
                if ( weights_.contains( key ) ) {
                        PANIC( "param key {} already registed.", key );
                }
                auto v   = std::make_unique<Weight>( key );
                auto ptr = v.get( );

                weights_.emplace( std::move( key ), std::move( v ) );
                return ptr;
        }

        /* Apply embedding (lookup) layer transformation on input. */
        auto applyEmbeddingLayer( Value *input, Value *table ) -> Value *
        {
                auto op = std::make_unique<Op<2>>(
                    OpKind::Gatter, std::initializer_list{ input, table } );
                auto ptr = op.get( );
                ops_.push_back( std::move( op ) );
                return ptr;
        }

        /* Apply dense layer transformation on input. */
        auto applyDenseLayer( Value *input, Value *weight ) -> Value *
        {
                auto op = std::make_unique<Op<2>>(
                    OpKind::Dense, std::initializer_list{ input, weight } );
                auto ptr = op.get( );
                ops_.push_back( std::move( op ) );
                return ptr;
        }

        /* Assert equal. */
        auto assertEqual( Value *expected, Value *got ) -> Value *
        {
                auto op = std::make_unique<Op<2>>(
                    OpKind::AssertEq, std::initializer_list{ expected, got } );
                auto ptr = op.get( );
                ops_.push_back( std::move( op ) );
                return ptr;
        }
};

}  // namespace aix

using namespace aix;

namespace {
struct AppCfg {
        bool quiet;
};

AppCfg
parseFlags( int argc, const char **argv )
{
        AppCfg cfg{ };
        if ( argc == 1 ) return cfg;

        if ( argc == 2 && 0 == std::strcmp( "--quiet", argv[1] ) ) {
                cfg.quiet = true;
        } else {
                PANIC( "unsupported flag: {}", argv[1] );
        }
        return cfg;
}
}  // namespace

int
main( int argc, const char **argv )
{
        auto cfg = parseFlags( argc, argv );
        auto p   = Program{ };

        auto x        = p.registerParam( "x" );
        auto expected = p.registerParam( "expected" );

        auto embedding = p.registerWeight( "embedding" );
        auto wq        = p.registerWeight( "wq" );
        auto out1      = p.applyEmbeddingLayer( x, embedding );
        auto out2      = p.applyDenseLayer( out1, wq );

        auto output = p.assertEqual( expected, out2 );

        if ( cfg.quiet ) {
                output->emit( );
                return 0;
        }

        std::print( "Program:\n" );
        std::system(
            std::format( "echo '{}' | jq --indent 7", output->getDebugJson( ) )
                .c_str( ) );
        std::print( "Emitted Bytecode:\n" );
        output->emit( );
        return 0;
}

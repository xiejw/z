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

#define PANIC( fmt, ... )                       \
        do {                                    \
                std::print( fmt, __VA_ARGS__ ); \
                std::abort( );                  \
        } while ( 0 )

#define VM_BYTE_CODE_PREFIX \
        "        CHECK_AND_JUMP( vm_program_push_op( program, "
#define VM_BYTE_CODE_SUFFIX " ) );\n"

namespace aix {

struct Value {
        virtual ~Value( ) = default;

        /* Returns the debug JSON string. */
        virtual auto getDebugJson( ) -> std::string = 0;

        /* Emit the VM bytecode assembly. */
        virtual auto emit( ) -> void = 0;
};

/* A named param represents a placeholder for one-off input, temporary param. */
struct NamedParam : public Value {
      public:
        NamedParam( std::string_view name ) : name_( name ) {}

        auto getDebugJson( ) -> std::string override
        {
                return std::format( R"({{ "type": "param", "name": "{}" }})",
                                    name_ );
        };

        auto emit( ) -> void override
        {
                std::print( VM_BYTE_CODE_PREFIX
                            "OP_LOAD_PARAM, {}" VM_BYTE_CODE_SUFFIX,
                            emitParamToLoad( ) );
        }

      protected:
        auto getParamName( ) const -> const std::string & { return name_; }
        auto emitParamToLoad( ) const -> const std::string
        {
                return std::format( "model->{0}.name, model->{0}.weight",
                                    name_ );
        }

      private:
        std::string name_;
};

/* A weight represents a long live model weight. */
struct Weight : public NamedParam {
      public:
        Weight( std::string_view name ) : NamedParam( name ) {}

        auto getDebugJson( ) -> std::string override
        {
                return std::format( R"({{ "type": "weight", "name": "{}" }})",
                                    getParamName( ) );
        };

        auto emit( ) -> void override
        {
                std::print( VM_BYTE_CODE_PREFIX
                            "OP_LOAD_WEIGHT, {}" VM_BYTE_CODE_SUFFIX,
                            emitParamToLoad( ) );
        }
};

enum class OpKind {
        Gatter,
};

template <int N>
struct Op : public Value {
      public:
        Op( OpKind kind, std::initializer_list<Value *> operands )
            : kind_( kind )
        {
                operands_.reserve( N );
                operands_.insert( operands_.end( ), operands );
        }

        auto getDebugJson( ) -> std::string override
        {
                switch ( kind_ ) {
                case OpKind::Gatter:
                        return std::format(
                            R"({{ "type": "op", "kind": "gatter", "embedding": {}, "input": {} }})",
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
        /* Creates a new named parameter. */
        auto createParam( std::string_view name ) -> Value *
        {
                auto key = std::string{ name };
                if ( params_.contains( key ) ) {
                        PANIC( "param key {} already registed.", key );
                }
                auto v   = std::make_unique<NamedParam>( key );
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

        /* Apply embedding transformation on input. */
        auto applyEmbedding( Value *weight, Value *input ) -> Value *
        {
                auto op = std::make_unique<Op<2>>(
                    OpKind::Gatter, std::initializer_list{ weight, input } );
                auto ptr = op.get( );
                ops_.push_back( std::move( op ) );
                return ptr;
        }
};

}  // namespace aix

using namespace aix;

int
main( )
{
        auto p         = Program{ };
        auto x         = p.createParam( "x" );
        auto embedding = p.registerWeight( "embedding" );
        auto output    = p.applyEmbedding( embedding, x );

        std::print( "Program:\n        {}\n", output->getDebugJson( ) );
        std::print( "Emitted Bytecode:\n" );
        output->emit( );
}

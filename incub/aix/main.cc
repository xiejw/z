#include <format>
#include <memory>
#include <print>
#include <string>
#include <string_view>

class Value {
      public:
        virtual std::string echo( ) = 0;
        virtual ~Value( )           = default;
};

struct StaticValue : public Value {
        StaticValue( std::string name ) : name( std::move( name ) ) {}
        std::string name;

        std::string echo( ) override { return name; }
};

struct DenseConfig {
        std::string_view w;
};

class Dense : public Value {
      public:
        Dense( const std::shared_ptr<Value> &in, DenseConfig &&cfg )
            : v_in( in ), w( cfg.w ) {};
        Value &getInput( ) { return *v_in; }

        std::string echo( ) override
        {
                return std::format( "dense with value `{}` with weight `{}`",
                                    v_in->echo( ), w );
        }

      private:
        std::shared_ptr<Value> v_in;
        std::string            w;
};

template <typename T>
constexpr std::shared_ptr<Value>
operator|( const std::shared_ptr<T> &value, DenseConfig &&cfg )
{
        return std::make_shared<Dense>( value, std::move( cfg ) );
}

class DenseAdaptor {
      public:
        constexpr static DenseConfig operator( )( std::string_view weight_name )
        {
                return DenseConfig{ weight_name };
        }
};

inline constexpr auto dense = DenseAdaptor{ };

int
main( )
{
        auto v = std::make_shared<StaticValue>( "static_v" );
        auto u = v | dense( "weight" ) | dense( "weight2" );
        std::print( "{}", u->echo( ) );
        std::print( "Hello world\n" );
}

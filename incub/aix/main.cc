#include <print>
#include <string>

class Value {};

class Dense : public Value {
      public:
        Dense( Value *in, std::string w ) : v_in( in ), w( std::move( w ) ) {};
        Value &getInput( ) { return *v_in; }

      private:
        Value      *v_in;
        std::string w;
};

class DenseBuilder;

class DenseBuilder {
      public:
        DenseBuilder( std::string w ) : w( std::move( w ) ) {};

      private:
        std::string w;

        template <typename T>
        friend constexpr Value *operator|( const T            &value,
                                           const DenseBuilder &builder )
        {
                auto v = new Dense{ value, builder.w };
                return v;
        }
};

class DenseAdaptor {
      public:
        constexpr DenseBuilder operator( )( std::string weight_name ) const
        {
                return DenseBuilder{ std::move( weight_name ) };
        }
};

inline constexpr auto dense = DenseAdaptor{ };

int
main( )
{
        auto v = new Value{ };
        auto u = v | dense( "weight" );
        (void)u;
        std::print( "Hello world\n" );
}

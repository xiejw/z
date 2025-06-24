#include <print>
#include <string>
#include <string_view>

class Value {};

class Dense : public Value {
      public:
        Dense( Value *in, std::string_view w ) : v_in( in ), w( w ) {};
        Value &getInput( ) { return *v_in; }

      private:
        Value      *v_in;
        std::string w;
};

class DenseBuilder {
      public:
        DenseBuilder( std::string_view w ) : w( w ) {};

      private:
        std::string_view w;

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
        constexpr static DenseBuilder operator( )(
            std::string_view weight_name )
        {
                return DenseBuilder{ weight_name };
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

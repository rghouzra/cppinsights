struct add_const
{
  template <typename C>
  struct to
  {
    template <typename T>
    struct inner {
      using type = T; 
    };
    template <typename ...T>
    using result = typename inner<T...>::type;
  };
};

add_const::to<int>::result<int> xx;

struct test
{
    template<typename T, int X>
    struct inner
    {
        template<char C, int F=4, typename U=int>
        struct winner
        {
            using type = T;
            using typeC = U;
        };

        using res = typename winner<3, 4, T>::type;

    };
};

test::inner<int, 2>::res rr;


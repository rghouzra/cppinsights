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

        template<typename U=int, typename... Ts>
        struct skinner
        {
            using type = U;
        };


        template<typename U=int, int... Ts>
        struct linner
        {
            using type = U;
        };

        using res = typename winner<3, 4, T>::type;
        using reskin = typename skinner<T, int, T>::type;
        using lreskin = typename linner<T, 3, 4>::type;

    };
};

test::inner<int, 2>::res rr;
test::inner<int, 2>::reskin rrs;
test::inner<int, 2>::lreskin lrrs;

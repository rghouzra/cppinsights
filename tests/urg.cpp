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

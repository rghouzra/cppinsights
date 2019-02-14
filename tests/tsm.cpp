template <typename T>
class DependentScopeMemberExprWrapper {
    public:
  T member;
};

class Foo
{

struct X
{
 template <typename T>
 void TestDependentScopeMemberExpr() {
   DependentScopeMemberExprWrapper<T> obj;
   obj.member = T();
   (&obj)->member = T();
 }
};

};


template<typename T>
void Baar()
{
}

int main()
{
}

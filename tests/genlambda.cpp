int main()
{
    int x;
    auto l = [](auto z) { return z+2; };

    return l(x);
}

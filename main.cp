namespace testNS {
  struct test {
    int b;

    void func();
  };
}


int main() {
  testNS.test a; /* you can use testNS. or testNS:: here */  

  a.func();
}
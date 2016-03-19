#include "stdafx.h"

#include <iostream>
#include <typeinfo>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_base_of.hpp>

template<typename T, void(T::*)(int) = &T::foo>
struct FooIndicator { typedef void type; };

template <typename, typename=void>
struct IsFooEnabledRecursive;

template<typename T, typename=void>
struct IsFooEnabled : IsFooEnabledRecursive<T> { };

template<typename T>
struct IsFooEnabled<T, typename FooIndicator<T>::type> : boost::true_type { };

template<typename, typename>
struct IsFooEnabledRecursive : boost::false_type { };

template<template <typename> class Derived, typename Base>
struct IsFooEnabledRecursive<Derived<Base>
   ,  typename boost::enable_if<boost::is_base_of<Base, Derived<Base> > >::type>
   :  IsFooEnabled<Base> { };

template <typename T> struct IfFooEnabled : boost::enable_if<IsFooEnabled<T> > { };
template <typename T> struct IfFooDisabled : boost::disable_if<IsFooEnabled<T> > { };

class TestBinding
{
public:
   void foo(int value) { std::cout << "foo has been called with " << value << std::endl; }
};

class TestBindingNoFoo
{
public:
};

template<class T>
class Accessor : public T
{
};

template<class T>
class TestFoo : public T
{
private:
   template <typename U>
   typename IfFooEnabled<U>::type doTest()
   {
      std::cout << "foo is enabled" << std::endl;
      T::foo(42); // non-virtual call
      this->foo(123);
   }

   template <typename U>
   typename IfFooDisabled<U>::type doTest()
   {
      std::cout << "foo is disabled" << std::endl;
      // T::foo(42); // error: 'foo' is not a member of 'Accessor<TestBindingNoFoo>'
      // this->foo(123); // error: 'foo' is not a member of 'Accessor<TestBindingNoFoo>'
   }

public:
   TestFoo()
   {
      doTest<T>();
      doTest<TestFoo>();
   }
};

int main(int argc, char* argv[])
{
   typedef TestFoo<Accessor<TestBinding> > TestFoo1;
   typedef TestFoo<Accessor<TestBindingNoFoo> > TestFoo2;

   TestFoo1 obj1;
   TestFoo2 obj2;
}


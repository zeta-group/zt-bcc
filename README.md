<kbd>bcc</kbd> is an enhanced ACS bytecode compiler for the ZDoom family of ports.

```
strict namespace SampleCode {
   script "Main" open {
      static str basket[] = { "apples", "oranges", "pears" };
      foreach ( auto fruit; basket ) {
         Print( s: "I love ", s: fruit, s: ( fruit == "oranges" ) ?
            " very much" : "" );
      }
   }
}
```

BCS is an extension of ACS. BCS is mostly compatible with ACS and provides many interesting and useful features, including the following:

* Structures
* Enumerations
* Namespaces
* Preprocessor
* Strong types
* Block scoping
* Optional function parameters
* Object and function references
* `&&` and `||` operators are short-circuited
* `foreach` loop
* `?:` operator

See the [wiki](https://github.com/zeta-group/zt-bcc/wiki) page for an overview of the features.
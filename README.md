# 🦆 Canard

## Introduction

Canard is a lightweight automatic theorem prover, based on type theory. In Canard, one can write definitions and theorems (as axioms), and search for examples and proofs.

## Program usage

```
Usage:
  canard [<options>] [<source files>]

Options:
  --help               Show help page.
  --threads <number>   Specify the amount of threads used for searching, by default 1.
  --depth <number>     Specify the maximum search depth, by default 5.
  --namespaces         Specify identifiers are printed with namespace.
  --json               Specify the output messages to be printed in JSON.
  --docs <path>        Write JSON documentation file.
  --defs <path>        Write JSON definition file.
```

## Language syntax

A canard file consists of a sequence of statements, which can be any of the following.

- `import <string>` imports the specified file. The path can be absolute, or relative with respect to the current file.

- `exit` exits the program.

- `let <function definition>` creates a new function in the current namespace, according to the given definition. More about this in the next section.

- `structure <structure definition>` creates a new function in the current namespace, according to the given definition. More about this in a later section.

- `search <telescope>` searches for examples/proofs of the functions in the telescope.

- `prove <identifier>` searches for a proof of the given function, where the proof must of course be independent of the function itself.

- `check <identifier>` prints the parameters and the type of the given function.

- `namespace <name>` sets the current namespace to the subspace in the current namespace with the given name. If no such subspace exists, it is created. 

- `end <name>` unsets the current namespace opened by `namespace` to what the current namespace was before. `<name>` should match whatever was provded to `namespace`.

- `open <namespace>` opens the provided namespace. The namespace must be provided in full, as to avoid ambiguity. An asterisk  `*` can be used to open all namespaces, and  e.g. `foo.bar.*` can be used to open all namespaces in `foo.bar`.

- `close <namespace>` closes the provided namespace. The same conventions apply as for `open`.

- `docs <identifier>` prints the documentation corresponding to the given function. Requires the `--docs` option to be set.

Line comments can be written using the `--` prefix, and block comments can be written using the `/-` prefix and `-/` suffix.

## Functions

The basic building blocks in Canard are _functions_. A function is an object consisting of the following data:

- a *name*: a string used to refer to the function.

- *parameters*: a telescope of functions, that is, a list of functions in which functions are allowed to depend on functions which appear earlier in the list.

- a *type*: a function whose type is `Type`.

Functions can be created using the `let` statement. Consider the following example. Algebraic structures, properties of such structures, and theorems about such, can all be represented by these functions.

```
-- Create a function whose name is 'Ring',
-- has no parameters, and whose type is `Type`.
let Ring : Type

-- Create a function whose name is `domain`, has one
-- parameter (a `Ring` named `R`), with type `Prop`
let domain (R : Ring) : Prop

-- Create the following functions, which represent
-- the fact that every domain is a field.
let field (R : Ring) : Prop
let domain_of_field {R : Ring} (h : domain R) : field R 
```
The terms `domain R` and `field R` are *specializations*. A specialization is an object that extends a *function*, having the following additional data:

- a *base*: a function which it is said to specialize.

- *arguments*: a list of functions that match the parameters of the base.

Specializations can be created by writing the base and arguments separated by spaces. In the above example, the bases of `domain R` and `field R` are `domain` and `field`, respectively, and both have a single argument `R`.

The curly brackets `{`, `}` in the line defining `domain_of_field` indicate that the parameter `R` is *implicit*, meaning no argument for it needs to be provided when specializing.

```
-- Create a ring called `QQ` and indicate it is a field.
let QQ : Ring
let QQ_domain : field QQ

-- `QQ_domain` can be applied to `domain_of_field`, without
-- needing to first provide `QQ`.
let QQ_field := domain_of_field QQ_domain
```

Note that a specialization can still have parameters itself. Consider the following example, where `ring_finite` is a specialization of `set_finite`, but has a parameter `R` itself as well.

```
let Set : Type
let to_set (R : Ring) : Set
let set_finite (S : Set) : Prop
let ring_finite (R : Ring) := set_finite (to_set R)
```

## Searching

One can search for functions of a certain type using the `search` statement. Provided should be a telescope of functions, similar to the parameters of a function. For example, consider the following code, where we search for a ring which is a domain.

```
let Ring : Type
let domain field (R : Ring) : Prop
let domain_of_field {R : Ring} (h : field R) : domain R

let QQ : Ring
let QQ_field : field QQ

search (R : Ring) (h : domain R)

```

Canard will output the following:

```
🔎 R = QQ, h = domain_of_field QQ_field
```

## Structures

*Structures* can be used to bundle data. Consider the following example which defines what an algebra over a ring is.

```
let Hom (R S : Ring) : Type

structure Algebra (k : Ring) := {
	ring : Ring,
	map : Hom k ring
}
```

Now, let us define, for every ring `R`, an identity homomorphism `id R`, and search for an algebra over `QQ`.

```
let id (R : Ring) : Hom R R

search (A : Algebra QQ)
```

Canard will output the following:

```
🔎 A = Algebra QQ { ring := QQ, map := id QQ }
```

Under the hood, defining a structure will actually define a type together with a constructor. In the case of `Algebra`, the following functions are created.

```
let Algebra (k : Ring) : Type
let Algebra.mk (k : Ring) (ring : Ring) (map : Hom k ring) : Algebra k
```

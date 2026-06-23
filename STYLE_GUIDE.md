# Code Style Guide

This repository uses [Prettier](https://prettier.io/) for supported web files. Run it from the repository root:

```sh
npm exec --prefix web -- prettier --write .
```

## Control flow

- Always wrap the body of an `if`, `else if`, or `else` statement in curly brackets.
- Never write an `if` statement and its body on one line.
- Keep `else if` and `else` on the same line as the preceding closing curly bracket.
- Add one blank line after the complete `if`/`else if`/`else` chain when another statement follows.
- A blank line is not required when the conditional is the final statement in its enclosing scope.

Preferred:

```ts
if (error) {
    error.textContent = '';
}

button.disabled = false;
```

```ts
if (response.ok) {
    handleSuccess();
} else {
    handleFailure();
}

recordAttempt();
```

Avoid:

```ts
if (error) error.textContent = '';
```

```ts
if (response.ok) {
    handleSuccess();
}
recordAttempt();
```

These control-flow rules apply to every language in the repository that uses curly-bracket blocks. Follow the language's normal syntax in Python and shell scripts.

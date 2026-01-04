
# Contributing to Bolt C++

Thank you for your interest in contributing to Bolt C++! This document provides guidelines and instructions for contributing to the project.

## Getting Started

1. Fork this Repl to create your own copy
2. Make your changes in your fork
3. Submit a well-documented pull request

## Code Style Guidelines

- Use consistent indentation (2 spaces)
- Follow C++17 standards and features
- Use meaningful variable and function names
- Document your code using Doxygen-style comments
- Keep functions focused and concise
- Use const correctness appropriately
- Implement RAII patterns where applicable

## Project Structure

```
bolt/
├── include/          # Header files
│   └── bolt/
│       ├── ai/      # AI-related headers
│       ├── core/    # Core functionality
│       ├── editor/  # Editor components
│       ├── network/ # Network handling
│       └── utils/   # Utility functions
├── src/             # Source files
├── test/            # Unit tests
├── docs/            # Documentation
└── examples/        # Example code
```

## Pull Request Process

1. Update documentation if needed
2. Add/update tests for new functionality
3. Ensure all tests pass
4. Update the README.md if needed
5. Reference any related issues

## Testing

- Write unit tests for new functionality
- Ensure existing tests pass
- Use googletest framework for C++ testing
- Test edge cases and error conditions

## Commit Messages

Format your commit messages as follows:
```
type(scope): description

[optional body]
[optional footer]
```

Types:
- feat: New feature
- fix: Bug fix
- docs: Documentation changes
- style: Code style changes
- refactor: Code refactoring
- test: Test changes
- chore: Build/maintenance changes

## Bug Reports

When filing a bug report, please include:
- Clear description of the issue
- Steps to reproduce
- Expected vs actual behavior
- Code samples if applicable
- Environment details

## Feature Requests

When proposing new features:
- Describe the problem you're solving
- Explain your proposed solution
- Provide example use cases
- Consider implementation complexity

## Questions or Need Help?

- Check existing documentation
- Search through issues
- Create a new issue with the question tag

## License

By contributing, you agree that your contributions will be licensed under the project's MIT License.

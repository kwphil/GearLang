# Error

## descripton

- this file creates and throws custom error codes and shows where the error has been made in the gearlang language

## functions


- `throw_error()` - is the main error hanlder - it looks for the code that error'd - prints it along with the highlights - if it can't find where the code error'd - then it calls `throw_error_no_highlight()` as a fall back option

- `setup_error_manager()` - is the function that opens a file for `throw_error()`



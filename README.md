Win32PrinterTest
================

A sample program to demonstrate how to use printer related API in Win32API, attached with project file of Visual C++ 6.0 and Visual Studio 2010. Perhaps can be compiled with other compiler.

Firstly this program shows a print-dialog. Then, if user selects a printer with other parameters and pressed OK, this program: 

- prints out a byte 'A' as the first page
- prints out a byte 'B' as the second page
- prints out a byte 'Z' as the 26th page
- prints out a pair of CR-LF for each end of copies 

Note that if the user checks "Print to file", this program shows a dialog to ask him/her a name of a file. In this case, the data to be printed will never sent to the selected printer but will be written to the file. Using this option makes this sample easier to understand.   
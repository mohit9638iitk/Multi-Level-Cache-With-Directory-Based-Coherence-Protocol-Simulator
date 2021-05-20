/* How to Run The Code */

Please make sure that you have follow all instruction mentioned 
in part1 of the assignment like compiling prog1.c etc in Assignment 2  .


Code for trace generator  is saved as 'tarce_gen.cpp'

1. To run the code for the 'tarce_gen.cpp' question :
   a). $ make obj-intel64/trace_gen.so 

       To make the trace the file you need to provide the program name 
       for example : prog4 and number of threads as 8,as given below :
   b). $ ../../../pin -t obj-intel64/f=trace_gen.so -- ./prog4 8
       
       It will generate a trace named as  'trace.out' .


Code of Stimulation  is saved as 'main.cpp'. Header file named as
'cache_header.h' is used by stimulation.
2. To run the code for the Stimulation part :
   a). compiled it as :
	$ g++ main.cpp -o main
  
   b). Run it for a particular trace file generated in step 1 as:
        $ ./main trace.out

It will output all the statistics required for Analysis Part.

       



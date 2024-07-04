# IBM650
Emulator for the IBM Type 650 computer

The IBM Type 650 is known as the first mass-produced computer.  It had no high-speed memory; all storage was on a magnetic drum that held 2000 10-digit words.  The storage locations of the program instructions had to be chosen carefully, otherwise execution time could be dominated by waiting for an address on the drum to roll around to the read head.  This was the programmer's responsibility.  Each instruction contained the address of the next instruction.  Selecting addresses that minimized drum access time was called "optimum programming" in the operator manual.

The operator manual and other documents from http://www.bitsavers.org/pdf/ibm/650/ was used to define the behavior of the system.  The documentation is very detailed, but some gaps needed to filled by guessing.  I have no experience with a real 650, so if I guessed wrong, please let me know.

The code in this project emulates the 650 instruction set, and also simulates the physical constraints of the system so that the effect of optimum programming can be seen.  A type 533 card reader and punch is simulated for input and output.

Development is being done in a test-driven style.  Currently all opcodes except read and punch are imlemented.  Output from the card reader and some input cases are implemented.  Some timing tests are in place, but a full set of timing tests needs to be written.  Error conditions reported by the 650 are also only partially implemented. 


This directory contains example programs for using the compiler to compile code to the 3D-FPIM architecture.

Set up environment to point to lib3dfpim.so:

    export LD_LIBRARY_PATH=`pwd`/../src:$LD_LIBRARY_PATH

Compile the examples:

    make                    # Compile all examples
    make <test-name>.test   # Compile a specific example

Execute the examples to generate the 3D-FPIM assembly code:

    ./<test-name>.test      # Execute a specific example

Generate PDF illustrations from .dot files (used for debugging)

    ./generate-pdf.sh

View compiler report

    cat <test-name>-report.out


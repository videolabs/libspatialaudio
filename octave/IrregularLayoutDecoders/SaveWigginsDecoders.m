% Decoders for 5.1 layouts supplied by Bruce Wiggins:
##Copyright 2017 Dr Bruce Wiggins
##bruce.wiggins@gmail.com
##http://www.brucewiggins.co.uk
##
##Permission is hereby granted, free of charge, to any person obtaining a copy of this
##software and associated documentation files (the "Software"), to deal in the Software
##without restriction, including without limitation the rights to use, copy, modify,
##merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
##permit persons to whom the Software is furnished to do so, subject to the following
##conditions:
##The above copyright notice and this permission notice shall be included in all copies
##or substantial portions of the Software.
##
##THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
##INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
##PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
##LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
##TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
##OR OTHER DEALINGS IN THE SOFTWARE.

% This script takes FuMa convention 5.1 decoders and converts them to AmbiX decoders
% Author: Peter Stitt
% Data: May 2024

clear
close all

% order 1 FuMa decoder
D_1 = [0.4250	0.3600 	0.4050	0.0;
    0.4250	0.3600 	-0.4050	0.0;
    0.4700	-0.3300	0.4150	0.0;
    0.4700	-0.3300	-0.4150	0.0;
    0.200  	0.1600 	0.0000	0.0];

% order 1 FuMa-to-AmbiX
D_1_50(:,1) = D_1(:,1)/sqrt(2);
D_1_50(:,2) = D_1(:,3);
D_1_50(:,3) = D_1(:,4);
D_1_50(:,4) = D_1(:,2);

D_1_51 = D_1_50;
D_1_51(end+1,1) = 0.5;

% order 2 FuMa decoder
D_2 = [0.4050  0.32000  0.31000  -0.00000  0.00000  0.00000   0.00000   0.08500   0.12500;
    0.4050  0.32000 -0.31000  -0.00000  0.00000 -0.00000   0.00000   0.08500  -0.12500;
    0.6350 -0.33500  0.28000  -0.00000 -0.00000  0.00000  -0.00000  -0.08000   0.08000;
    0.6350 -0.33500 -0.28000  -0.00000 -0.00000  0.00000  -0.00000  -0.08000  -0.08000;
    0.0850  0.04000  0.0       0.00000  0.0      0.00000   0.0       0.04500   0.0];

% order 2 FuMa-to-AmbiX
D_2_50(:,1) = D_2(:,1)/sqrt(2);
D_2_50(:,2) = D_2(:,3);
D_2_50(:,3) = D_2(:,4);
D_2_50(:,4) = D_2(:,2);
D_2_50(:,5) = D_2(:,9)/(sqrt(3)/2);
D_2_50(:,6) = D_2(:,7)/(sqrt(3)/2);
D_2_50(:,7) = D_2(:,5);
D_2_50(:,8) = D_2(:,6)/(sqrt(3)/2);
D_2_50(:,9) = D_2(:,8)/(sqrt(3)/2);

D_2_51 = D_2_50;
D_2_51(end+1,1) = 0.5;

% Order 3 FuMa decoder
D_3 = [0.3100 0.2850 0 0.3100 0.1850 0 0 0 0.0050 0.0600 0 0 0 0 0 -0.0250;
    0.3100 -0.2850 0 0.3100 -0.1850 0 0 0 0.0050 -0.0600 0 0 0 0 0 -0.0250;
    0.5900 0.3850 0 -0.3350 0.0200 0 0 0 -0.0250 -0.0650 0 0 0 0 0 0.0250;
    0.5900 -0.3850 0 -0.3350 -0.0200 0 0 0 -0.0250 0.0650 0 0 0 0 0 0.0250;
    0.1350 0 0 0.2650 0 0 0 0 0.2150 0 0 0 0 0 0 0.1050    ];

% Order 3 FuMa-to-AmbiX
D_3_50(:,1) = D_3(:,1)/sqrt(2);
D_3_50(:,2) = D_3(:,2);
D_3_50(:,3) = D_3(:,3);
D_3_50(:,4) = D_3(:,4);
D_3_50(:,5) = D_3(:,5)/(sqrt(3)/2);
D_3_50(:,6) = D_3(:,6)/(sqrt(3)/2);
D_3_50(:,7) = D_3(:,7);
D_3_50(:,8) = D_3(:,8)/(sqrt(3)/2);
D_3_50(:,9) = D_3(:,9)/(sqrt(3)/2);
D_3_50(:,10) = D_3(:,10)/sqrt(5/8);
D_3_50(:,11) = D_3(:,11)/(sqrt(5)/3);
D_3_50(:,12) = D_3(:,12)/sqrt(32/45);
D_3_50(:,13) = D_3(:,13);
D_3_50(:,14) = D_3(:,14)/sqrt(32/45);
D_3_50(:,15) = D_3(:,15)/(sqrt(5)/3);
D_3_50(:,16) = D_3(:,16)/sqrt(5/8);

D_3_51 = D_3_50;
D_3_51(end+1,1) = 0.5;

save('decodingMatrices_5p0_AmbiX.mat','D_1_50','D_2_50','D_3_50')
save('decodingMatrices_5p1_AmbiX.mat','D_1_51','D_2_51','D_3_51')

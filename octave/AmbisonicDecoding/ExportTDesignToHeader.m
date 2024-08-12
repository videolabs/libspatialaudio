% Take the 5200 t-design from and convert it to azimuth/elevation instead of
% azimuth/inclination i.e. convert the 2nd column from 0->pi (top->bottom)
% to -pi/2->pi/2 (bottom->top).
% Save it in a header file.
%
% t-design by Dr. Manuel Gräf:
% https://www-user.tu-chemnitz.de/~potts/workgroup/graef/quadrature/index.php.en
clear
close all
clc

addpath('../Export/');

t5200 = load('Design_5200_100_random.dat');
t5200(:,2) = pi/2 - t5200(:,2);

fID = fopen('../../include/t_design_5200.h','w');

fwrite(fID,['// 5200-point Chebyshev-type Quadrature sampling on the sphere from: https://www-user.tu-chemnitz.de/~potts/workgroup/graef/quadrature/index.php.en' char(10) char(10)]);

fwrite(fID,['#pragma once' char(10) char(10)])
fwrite(fID,['namespace tDesign5200 {' char(10)]);

% Build array string.
arrayStr = '{';

for c = 1:size(t5200, 1)
 arrayStr = [arrayStr, '{'];
 for i = 1:size(t5200, 2)
   elem = t5200(c, i);
   arrayStr = [arrayStr num2str(elem,'%f') 'f, '];
 end
 arrayStr = arrayStr(1:length(arrayStr) - 2);
 arrayStr = [arrayStr, '},', char(10)];
end

arrayStr = arrayStr(1:length(arrayStr) - 2);
arrayStr = [arrayStr, '};'];

% Write array declaration to file.
fprintf(fID,'static constexpr float points[%i][%i] = %s\n\n', size(t5200,1), size(t5200, 2), arrayStr);

fwrite(fID,['    static constexpr unsigned int nTdesignPoints = ' num2str(size(t5200,1)) ';' char(10)]);
fwrite(fID,['};' char(10) char(10)]);

fclose(fID);

function [] = hullToHeader(fID,mat,varName)

   % Build array string.
   arrayStr = '{';

   for c = 1:length(mat)
     if mod(c,4) == 1
       arrayStr = [arrayStr, char(10)];
     end
     arrayStr = [arrayStr, '{'];
     for i = 1:length(mat{c})
       elem = mat{c}(i) - 1; % Subtract 1 to convert from Octave index to C++ index
       arrayStr = [arrayStr num2str(elem,'%d') ', '];
     end
     arrayStr = arrayStr(1:length(arrayStr) - 2);
     arrayStr = [arrayStr, '},'];
   end

   arrayStr = arrayStr(1:length(arrayStr) - 1);
   arrayStr = [arrayStr, '};'];

   % Write array declaration to file.
   fprintf(fID,'const std::vector<std::vector<unsigned int>> %s = %s\n\n', varName, arrayStr);

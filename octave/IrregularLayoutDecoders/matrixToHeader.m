function [] = matrixToHeader(fID,mat,varName)

   % Build array string.
   arrayStr = '{';

   for c = 1:size(mat, 2)
     arrayStr = [arrayStr, '{'];
     for i = 1:size(mat, 1)
       elem = mat(i, c);
       arrayStr = [arrayStr num2str(elem,'%f') 'f, '];
     end
     arrayStr = arrayStr(1:length(arrayStr) - 2);
     arrayStr = [arrayStr, '},', char(10)];
   end

   arrayStr = arrayStr(1:length(arrayStr) - 2);
   arrayStr = [arrayStr, '};'];

   % Write array declaration to file.
   fprintf(fID,'const float %s[][%i] = %s\n\n', varName, size(mat, 1), arrayStr);

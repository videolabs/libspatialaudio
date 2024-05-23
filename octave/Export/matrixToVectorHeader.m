function [] = matrixToVectorHeader(fID,mat,varName)

   % Build array string.
   arrayStr = '{';

   for c = 1:size(mat, 2)
     arrayStr = [arrayStr, '{', char(10)];
     for i = 1:size(mat, 1)
       elem = mat{i}, c);
       arrayStr = [arrayStr num2str(elem,'%d') ', '];
     end
     arrayStr = arrayStr(1:length(arrayStr) - 2);
     arrayStr = [arrayStr, '},'];
     if mod(c,4) == 0
       arrayStr = [arrayStr, char(10)];
     end
   end

   arrayStr = arrayStr(1:length(arrayStr) - 2);
   arrayStr = [arrayStr, '};'];

   % Write array declaration to file.
   fprintf(fID,'const std::vector<std::vector<unsigned int>> %s = %s\n\n', varName, size(mat, 1), arrayStr);

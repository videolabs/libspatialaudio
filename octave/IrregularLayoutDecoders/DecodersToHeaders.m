% Output decoders to c++ headers for inclusion
clear
close all

load('decodingMatrices_5p0_AmbiX.mat');
load('decodingMatrices_7p0_AmbiX.mat');

% Add LFE as the last channel to decoders
D_1_50(end+1,1) = 0.5;
D_2_50(end+1,1) = 0.5;
D_3_50(end+1,1) = 0.5;
D_1_70(end+1,1) = 0.5;
D_2_70(end+1,1) = 0.5;
D_3_70(end+1,1) = 0.5;

fID = fopen('decoderMatrices.h','w');

matrixToHeader(fID,D_1_50.','decoder_coefficient_first_5_1');
matrixToHeader(fID,D_2_50.','decoder_coefficient_second_5_1');
matrixToHeader(fID,D_3_50.','decoder_coefficient_third_5_1');
matrixToHeader(fID,D_1_70.','decoder_coefficient_first_7_1');
matrixToHeader(fID,D_2_70.','decoder_coefficient_second_7_1');
matrixToHeader(fID,D_3_70.','decoder_coefficient_third_7_1');

fclose(fID);


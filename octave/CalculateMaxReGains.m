% Calculate the max rE gains for psychoacoustic optimisation filters
clear
close all
clc

% Maximum order
maxN = 3;

% Calculate the 3D max-rE gains using equation (10) in:
% F. Zotter and M. Frank, “All-round ambisonic panning and decoding,” J. Audio Eng. Soc., vol. 60, no. 10, pp. 807–820, 2012.
gMaxRe3D = zeros(maxN, maxN+1);
EmaxRe3D = zeros(maxN,1);
Ebasic3D = zeros(maxN,1);
gMaxRe2D = zeros(maxN, maxN+1);
EmaxRe2D = zeros(maxN,1);
Ebasic2D = zeros(maxN,1);
for N = 1:maxN
  for n = 0:N
    % 3D gains
    gTmp = legendre(n, cos(137.9*(pi/180)/(N+1.51)));
    gMaxRe3D(N, n+1) = gTmp(1);
    EmaxRe3D(N) = EmaxRe3D(N) + (2*n+1)*gMaxRe3D(N, n+1)^2;
    Ebasic3D(N) = Ebasic3D(N) + (2*n+1);

    % 2D gains
    gMaxRe2D(N, n+1) = cos(n*pi/(2*N + 2));
    EmaxRe2D(N) = EmaxRe2D(N) + min((2*n+1),2)*gMaxRe2D(N, n+1)^2;
    Ebasic2D(N) = Ebasic2D(N) + min((2*n+1),2);
  end
end

% Compensate the gain to preserve the total energy
gMaxRe3D = gMaxRe3D .* sqrt(Ebasic3D./EmaxRe3D)
gMaxRe2D = gMaxRe2D .* sqrt(Ebasic2D./EmaxRe2D)


function cartCoords = fibonacciSphere(nPoints)
% Calculate the coordinates of points on a fibonacci sphere.
% % The algorithm and some extra reading can be found here:
% http://extremelearning.com.au/how-to-evenly-distribute-points-on-a-sphere-more-effectively-than-the-canonical-fibonacci-lattice/

goldenRatio = (1 + 5^(0.5))/2;
i = linspace(0,nPoints-1,nPoints);
theta = 2 *pi * i / goldenRatio;
phi = acos(1 - 2*(i+0.5)/nPoints);
cartCoords = [cos(theta.') .* sin(phi.'), sin(theta.') .* sin(phi.'), cos(phi.')];

end

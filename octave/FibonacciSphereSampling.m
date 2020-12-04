% Test the Fibonacci lattice algorithm on a sphere
% The algorithm and some extra reading can be found here:
% http://extremelearning.com.au/how-to-evenly-distribute-points-on-a-sphere-more-effectively-than-the-canonical-fibonacci-lattice/
clear
close all
clc

% Number of points to sample on the sphere
nPoints = 1500;
cartCoords = fibonacciSphere(nPoints);
polCoords = cart2sph(cartCoords(:,1),cartCoords(:,2),cartCoords(:,3));

figure(1)
subplot(1,2,1)
plot3(cartCoords(:,1),cartCoords(:,2),cartCoords(:,3),'.','MarkerSize',20)
title('Fibonacci')
axis equal
xlabel('x')
ylabel('y')
zlabel('z')

figure(2)
subplot(1,2,1)
plot(polCoords(:,1)/pi*180,polCoords(:,2)/pi*180,'.','MarkerSize',20)
xlabel('azimuth (degrees)')
ylabel('elevation (degrees)')
axis([-180, 180, -90, 90])

%% Compare this to the algorithm used by libear
nRows = 37;
el = linspace(-90,90,nRows);

perimiter_centre = 2 * pi;
positions = [];
for iEl = 1:length(el)
    radius = cos(el(iEl)/180*pi);
    perimiter = 2 * pi * radius;
    
    nPoints = round((perimiter / perimiter_centre) * 2 * (nRows - 1));
    if nPoints == 0
        nPoints = 1;
    end
    
    az = linspace(0,360-360/nPoints,nPoints);
    for iAz = 1:length(az)
        [posTmp(1),posTmp(2),posTmp(3)] = cart([az(iAz), el(iEl), 1.0]);
        positions = [positions; posTmp];
    end
end

[polarPositions(:,1), polarPositions(:,2), polarPositions(:,3)] = pol([positions(:,1),positions(:,2),positions(:,3)]);

figure(1)
subplot(1,2,2)
plot3(positions(:,1),positions(:,2),positions(:,3),'r.','MarkerSize',20)
title('Libear')
axis equal
xlabel('x')
ylabel('y')
zlabel('z')

figure(2)
subplot(1,2,2)
plot(polarPositions(:,1),polarPositions(:,2),'.','MarkerSize',20)
xlabel('azimuth (degrees)')
ylabel('elevation (degrees)')
axis([-180, 180, -90, 90])

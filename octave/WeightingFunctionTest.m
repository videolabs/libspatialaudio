% The weighting function of the ADM renderer
clear
close all
clc

nPoints = 1500;

% The width and height of the weighting function in degrees
width = 90;
height = 0;
% The fade out distance in degrees
fade_out = 10;

% virtual source positions
%virtualSourcePos = fibonacciSphere(nPoints);
nRows = 37;
el = linspace(-90,90,nRows);

perimiter_centre = 2 * pi;
virtualSourcePos = [];
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
        virtualSourcePos = [virtualSourcePos; posTmp];
    end
end
nPoints = size(virtualSourcePos,1);

[azOrig,elOrig,~] = pol(virtualSourcePos);

% panning direction where +ve y is the front and +ve x is the right, as defined in ADM standard
position = [0;1;0];
position = position/norm(position);

% Calculate the rotation matrix to go from a position to the panning direction basis
[az,el,d] = pol(position.');
[rotMat(1,1), rotMat(1,2), rotMat(1,3)] = cart([az - 90;0;1]);
[rotMat(2,1), rotMat(2,2), rotMat(2,3)] = cart([az;el;1]);
[rotMat(3,1), rotMat(3,2), rotMat(3,3)] = cart([az;el+90;1]);

if height > width
    rotMat = flipud(rotMat);
    width_ = width;
    width = height;
    height = width_;
end

% Handle the case where width > 180 so that they meet at the back
if width > 180
      width = 180 + (width - 180)/180*(180+height);
end

% Get the coordinates of the centre of the cap circles such
% that the edge of the circle is at width/2 when width < 180deg
circleCoordsPol = [width/2 - height/2, 0,1];
[circleCoordsCart(1,1),circleCoordsCart(1,2),circleCoordsCart(1,3)] = cart([width/2 - height/2, 0,1]);
[circleCoordsCart(2,1),circleCoordsCart(2,2),circleCoordsCart(2,3)] = cart([-(width/2 - height/2), 0,1]);

% Calculate the weight over all virtual source directions
for iVS = 1:nPoints
    % Calculate the direction of the virtual source in the panning basis
    positionBasis = rotMat*virtualSourcePos(iVS,:).';
    % Calculate the azimuth and elevation of the source in this basis
    [az,el,~] = pol(positionBasis.');
    azBasis(iVS) = az;
    elBasis(iVS) = el;
    % If the absolute azimuth is less than the circle azimuth coordinates
    % then calculate the distance using the elevation angle
    if abs(az) < circleCoordsPol(1)
        distance = abs(el) - height/2;
    else
        % Get the circle in the same hemisphere as the virtual source
        %if az >= 0
            closestCircle = 1;
        %elseif az < 0
        %    closestCircle = 2;
        %end
        if positionBasis(1) > 0
            positionBasis(1) = -positionBasis(1);
        end
        % If the azimuth is outside this rectangle then calculate its distance
        % from the closes of the circles
        distance = acos(dot(positionBasis,circleCoordsCart(closestCircle,:))/(norm(positionBasis)*norm(circleCoordsCart(closestCircle,:))))/pi*180 - height/2;
    end
    w(iVS) = min(max(distance,0),fade_out);
    w(iVS) = (1-w(iVS)/fade_out);
end

%% Plot the weights in the weighting bases
tri=delaunay(azBasis,elBasis);
figure(1)
trisurf(tri,azBasis,elBasis,w,'LineStyle','none')
axis equal
grid on
shading interp
xlabel('azimuth')
ylabel('elevation')
zlabel('weight')
set(gca,'XTick',-180:30:180)
set(gca,'YTick',-90:30:90)
view([0,90])

%% Plot the weights in the original coordinate system
tri=delaunay(azOrig,elOrig);
figure(2)
trisurf(tri,azOrig,elOrig,w,'LineStyle','none')
axis equal
grid on
shading interp
xlabel('azimuth')
ylabel('elevation')
zlabel('weight')
set(gca,'XTick',-180:30:180)
set(gca,'YTick',-90:30:90)
view([0,90])
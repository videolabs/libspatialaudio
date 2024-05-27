% Export the convex hulls for each of the loudspeaker layouts supported by ADMRenderer
%
% This script uses the OctaveForge package matgeom. If you are working in Matlab
% you can download this pack here: https://github.com/mattools/matGeom
%
% Author: Peter Stitt
% Date: May 2024
clear
close all
clc

% Set to true to export hulls to c++ header format
F_save = true;

filename = 'hulls.h';

pkg load matgeom

% Define layouts that include the speakers included in the layout and all of the
% extra speakers added by CalculateExtraSpeakersLayout

% Quad - not in ITU-R BS.2051-3
layout_0_4_0 = [45,0,1; -45,0,1; 135,0,1; -135,0,1;...
              45,-30,1; -45,-30,1; 135,-30,1; -135,-30,1;...
              45,30,1; -45,30,1; 135,30,1; -135,30,1;...
              0,-90,1; 0,90,1];
[layout_0_4_0_cart(:,1),layout_0_4_0_cart(:,2),layout_0_4_0_cart(:,3)] = ...
    sph2cart(layout_0_4_0(:,1)/180*pi,layout_0_4_0(:,2)/180*pi,layout_0_4_0(:,3));
% Convex hull
hull_0_4_0 = minConvexHull(layout_0_4_0_cart);
figure
drawMesh(layout_0_4_0_cart, hull_0_4_0)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('Quad')

% 0+5+0 - System B
layout_0_5_0 = [30,0,1; -30,0,1; 0,0,1; 110,0,1; -110,0,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 110,-30,1; -110,-30,1;...
              30,30,1; -30,30,1; 0,30,1; 110,30,1; -110,30,1;...
              0,-90,1; 0,90,1];
[layout_0_5_0_cart(:,1),layout_0_5_0_cart(:,2),layout_0_5_0_cart(:,3)] = ...
sph2cart(layout_0_5_0(:,1)/180*pi,layout_0_5_0(:,2)/180*pi,layout_0_5_0(:,3));
% Convex hull
hull_0_5_0 = minConvexHull(layout_0_5_0_cart);
figure
drawMesh(layout_0_5_0_cart, hull_0_5_0)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('0+5+0')

% 2+5+0 - System C
layout_2_5_0 = [30,0,1; -30,0,1; 0,0,1; 110,0,1; -110,0,1; 30,30,1; -30,30,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 110,-30,1; -110,-30,1;...
              110,30,1; -110,30,1;...
              0,-90,1; 0,90,1];
[layout_2_5_0_cart(:,1),layout_2_5_0_cart(:,2),layout_2_5_0_cart(:,3)] = ...
sph2cart(layout_2_5_0(:,1)/180*pi,layout_2_5_0(:,2)/180*pi,layout_2_5_0(:,3));
% Convex hull
hull_2_5_0 = minConvexHull(layout_2_5_0_cart);
figure
drawMesh(layout_2_5_0_cart, hull_2_5_0)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('2+5+0')

% 4+5+0 - System D
layout_4_5_0 = [30,0,1; -30,0,1; 0,0,1; 110,0,1; -110,0,1; 30,30,1; -30,30,1; 110,30,1; -110,30,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 110,-30,1; -110,-30,1;...
              0,-90,1; 0,90,1];
[layout_4_5_0_cart(:,1),layout_4_5_0_cart(:,2),layout_4_5_0_cart(:,3)] = ...
sph2cart(layout_4_5_0(:,1)/180*pi,layout_4_5_0(:,2)/180*pi,layout_4_5_0(:,3));
% Convex hull
hull_4_5_0 = minConvexHull(layout_4_5_0_cart);
figure
drawMesh(layout_4_5_0_cart, hull_4_5_0)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('4+5+0')

% 4+5+1 - System E
layout_4_5_1 = [30,0,1; -30,0,1; 0,0,1; 110,0,1; -110,0,1; 30,30,1; -30,30,1; 110,30,1; -110,30,1; 0,-30,1;...
              110,-30,1; -110,-30,1;...
              0,-90,1; 0,90,1];
[layout_4_5_1_cart(:,1),layout_4_5_1_cart(:,2),layout_4_5_1_cart(:,3)] = ...
sph2cart(layout_4_5_1(:,1)/180*pi,layout_4_5_1(:,2)/180*pi,layout_4_5_1(:,3));
% Convex hull
hull_4_5_1 = minConvexHull(layout_4_5_1_cart);
figure
drawMesh(layout_4_5_1_cart, hull_4_5_1)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('4+5+1')

% 3+7+0 - System F
layout_3_7_0 = [0,0,1; 30,0,1; -30,0,1; 45,30,1; -45,30,1; 90,0,1; -90,0,1; 135,0,1; -135,0,1; 180,45,1;...
              0,-30,1; 30,-30,1; -30,-30,1; 90,-30,1; -90,-30,1; 135,-30,1; -135,-30,1;...
              0,-90,1];
[layout_3_7_0_cart(:,1),layout_3_7_0_cart(:,2),layout_3_7_0_cart(:,3)] = ...
sph2cart(layout_3_7_0(:,1)/180*pi,layout_3_7_0(:,2)/180*pi,layout_3_7_0(:,3));
% Convex hull
hull_3_7_0 = minConvexHull(layout_3_7_0_cart);
figure
drawMesh(layout_3_7_0_cart, hull_3_7_0)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('3+7+0')

% 4+9+0 - System G with narrow screen speakers
layout_4_9_0 = [30,0,1; -30,0,1; 0,0,1; 90,0,1; -90,0,1; 135,0,1; -135,0,1; 45,30,1; -45,30,1; 135,30,1; -135,30,1; 15,0,1; -15,0,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 90,-30,1; -90,-30,1; 135,-30,1; -135,-30,1; 15,-30,1; -15,-30,1;...
              0,-90,1; 0,90,1];
[layout_4_9_0_cart(:,1),layout_4_9_0_cart(:,2),layout_4_9_0_cart(:,3)] = ...
sph2cart(layout_4_9_0(:,1)/180*pi,layout_4_9_0(:,2)/180*pi,layout_4_9_0(:,3));
% Convex hull
hull_4_9_0 = minConvexHull(layout_4_9_0_cart);
figure
drawMesh(layout_4_9_0_cart, hull_4_9_0)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('4+9+0')

% 4+9+0 - System G with wide screen speakers
layout_4_9_0_wide = [30,0,1; -30,0,1; 0,0,1; 90,0,1; -90,0,1; 135,0,1; -135,0,1; 45,30,1; -45,30,1; 135,30,1; -135,30,1; 45,0,1; -45,0,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 90,-30,1; -90,-30,1; 135,-30,1; -135,-30,1; 45,-30,1; -45,-30,1;...
              0,-90,1; 0,90,1];
[layout_4_9_0_wide_cart(:,1),layout_4_9_0_wide_cart(:,2),layout_4_9_0_wide_cart(:,3)] = ...
sph2cart(layout_4_9_0_wide(:,1)/180*pi,layout_4_9_0_wide(:,2)/180*pi,layout_4_9_0_wide(:,3));
% Convex hull
hull_4_9_0_wide = minConvexHull(layout_4_9_0_wide_cart);
figure
drawMesh(layout_4_9_0_wide_cart, hull_4_9_0_wide)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('4+9+0(wide)')

% 4+9+0 - System G with wide left screen speaker
layout_4_9_0_wideL = [30,0,1; -30,0,1; 0,0,1; 90,0,1; -90,0,1; 135,0,1; -135,0,1; 45,30,1; -45,30,1; 135,30,1; -135,30,1; 45,0,1; -15,0,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 90,-30,1; -90,-30,1; 135,-30,1; -135,-30,1; 45,-30,1; -15,-30,1;...
              0,-90,1; 0,90,1];
[layout_4_9_0_wideL_cart(:,1),layout_4_9_0_wideL_cart(:,2),layout_4_9_0_wideL_cart(:,3)] = ...
sph2cart(layout_4_9_0_wideL(:,1)/180*pi,layout_4_9_0_wideL(:,2)/180*pi,layout_4_9_0_wideL(:,3));
% Convex hull
hull_4_9_0_wideL = minConvexHull(layout_4_9_0_wideL_cart);
figure
drawMesh(layout_4_9_0_wideL_cart, hull_4_9_0_wideL)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('4+9+0(wide left)')

% 4+9+0 - System G with wide right screen speaker
layout_4_9_0_wideR = [30,0,1; -30,0,1; 0,0,1; 90,0,1; -90,0,1; 135,0,1; -135,0,1; 45,30,1; -45,30,1; 135,30,1; -135,30,1; 15,0,1; -45,0,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 90,-30,1; -90,-30,1; 135,-30,1; -135,-30,1; 15,-30,1; -45,-30,1;...
              0,-90,1; 0,90,1];
[layout_4_9_0_wideR_cart(:,1),layout_4_9_0_wideR_cart(:,2),layout_4_9_0_wideR_cart(:,3)] = ...
sph2cart(layout_4_9_0_wideR(:,1)/180*pi,layout_4_9_0_wideR(:,2)/180*pi,layout_4_9_0_wideR(:,3));
% Convex hull
hull_4_9_0_wideR = minConvexHull(layout_4_9_0_wideR_cart);
figure
drawMesh(layout_4_9_0_wideR_cart, hull_4_9_0_wideR)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('4+9+0(wide right)')


% 9+10+3 - System H
layout_9_10_3 = [60,0,1; -60,0,1; 0,0,1; 135,0,1; -135,0,1; 30,0,1; -30,0,1; 180,0,1; 90,0,1; -90,0,1;...
              45,30,1; -45,30,1; 0,30,1; 0,90,1; 135,30,1; -135,30,1;  90,30,1; -90,30,1; 180,30,1;...
              0,-30,1; 45,-30,1; -45,-30,1;...
              135,-30,1; -135,-30,1; 180,-30,1; 90,-30,1; -90,-30,1;...
              0,-90,1];
[layout_9_10_3_cart(:,1),layout_9_10_3_cart(:,2),layout_9_10_3_cart(:,3)] = ...
sph2cart(layout_9_10_3(:,1)/180*pi,layout_9_10_3(:,2)/180*pi,layout_9_10_3(:,3));
% Convex hull
hull_9_10_3 = minConvexHull(layout_9_10_3_cart);
figure
drawMesh(layout_9_10_3_cart, hull_9_10_3)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('9+10+3')

% 0+7+0 - System I
layout_0_7_0 = [30,0,1; -30,0,1; 0,0,1; 90,0,1; -90,0,1; 135,0,1; -135,0,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 90,-30,1; -90,-30,1; 135,-30,1; -135,-30,1;...
              30,30,1; -30,30,1; 0,30,1; 90,30,1; -90,30,1; 135,30,1; -135,30,1;...
              0,-90,1; 0,90,1];
[layout_0_7_0_cart(:,1),layout_0_7_0_cart(:,2),layout_0_7_0_cart(:,3)] = ...
sph2cart(layout_0_7_0(:,1)/180*pi,layout_0_7_0(:,2)/180*pi,layout_0_7_0(:,3));
% Convex hull
hull_0_7_0 = minConvexHull(layout_0_7_0_cart);
figure
drawMesh(layout_0_7_0_cart, hull_0_7_0)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('0+7+0')

% 4+7+0 - System J
layout_4_7_0 = [30,0,1; -30,0,1; 0,0,1; 90,0,1; -90,0,1; 135,0,1; -135,0,1; 45,30,1; -45,30,1; 135,30,1; -135,30,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 90,-30,1; -90,-30,1; 135,-30,1; -135,-30,1;...
              0,-90,1; 0,90,1];
[layout_4_7_0_cart(:,1),layout_4_7_0_cart(:,2),layout_4_7_0_cart(:,3)] = ...
sph2cart(layout_4_7_0(:,1)/180*pi,layout_4_7_0(:,2)/180*pi,layout_4_7_0(:,3));
% Convex hull
hull_4_7_0 = minConvexHull(layout_4_7_0_cart);
figure
drawMesh(layout_4_7_0_cart, hull_4_7_0)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('4+7+0')

% 2+7+0 - 7.1.2
layout_2_7_0 = [30,0,1; -30,0,1; 0,0,1; 90,0,1; -90,0,1; 135,0,1; -135,0,1; 45,30,1; -45,30,1;...
              30,-30,1; -30,-30,1; 0,-30,1; 90,-30,1; -90,-30,1; 135,-30,1; -135,-30,1;...
              90,30,1; -90,30,1; 135,30,1; -135,30,1;...
              0,-90,1; 0,90,1];
[layout_2_7_0_cart(:,1),layout_2_7_0_cart(:,2),layout_2_7_0_cart(:,3)] = ...
sph2cart(layout_2_7_0(:,1)/180*pi,layout_2_7_0(:,2)/180*pi,layout_2_7_0(:,3));
% Convex hull
hull_2_7_0 = minConvexHull(layout_2_7_0_cart);
figure
drawMesh(layout_2_7_0_cart, hull_2_7_0)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('2+7+0')

% 9+10+5 - EBU Tech 3369 (BEAR) 9+10+5 - 9+10+3 with B+135 & B-135 added
layout_9_10_5 = [60,0,1; -60,0,1; 0,0,1; 135,0,1; -135,0,1; 30,0,1; -30,0,1; 180,0,1; 90,0,1; -90,0,1;...
              45,30,1; -45,30,1; 0,30,1; 0,90,1; 135,30,1; -135,30,1;  90,30,1; -90,30,1; 180,30,1;...
              0,-30,1; 45,-30,1; -45,-30,1; 135,-30,1; -135,-30,1;...
              180,-30,1;...
              0,-90,1];
[layout_9_10_5_cart(:,1),layout_9_10_5_cart(:,2),layout_9_10_5_cart(:,3)] = ...
sph2cart(layout_9_10_5(:,1)/180*pi,layout_9_10_5(:,2)/180*pi,layout_9_10_5(:,3));
% Convex hull
hull_9_10_5 = minConvexHull(layout_9_10_5_cart);
figure
drawMesh(layout_9_10_5_cart, hull_9_10_5)
axis equal
xlabel('x'); ylabel('y'); zlabel('z');
title('9+10+5')

if F_save
  fID = fopen(filename,'w');

  hullToHeader(fID,hull_0_4_0,'HULL_0_4_0');
  hullToHeader(fID,hull_0_5_0,'HULL_0_5_0');
  hullToHeader(fID,hull_2_5_0,'HULL_2_5_0');
  hullToHeader(fID,hull_4_5_0,'HULL_4_5_0');
  hullToHeader(fID,hull_4_5_1,'HULL_4_5_1');
  hullToHeader(fID,hull_3_7_0,'HULL_3_7_0');
  hullToHeader(fID,hull_4_9_0,'HULL_4_9_0');
  hullToHeader(fID,hull_4_9_0_wide,'HULL_4_9_0_wide');
  hullToHeader(fID,hull_4_9_0_wideL,'HULL_4_9_0_wideL');
  hullToHeader(fID,hull_4_9_0_wideR,'HULL_4_9_0_wideR');
  hullToHeader(fID,hull_9_10_3,'HULL_9_10_3');
  hullToHeader(fID,hull_0_7_0,'HULL_0_7_0');
  hullToHeader(fID,hull_4_7_0,'HULL_4_7_0');
  hullToHeader(fID,hull_2_7_0,'HULL_2_7_0');
  hullToHeader(fID,hull_9_10_5,'HULL_9_10_5');

  fclose(fID);
end

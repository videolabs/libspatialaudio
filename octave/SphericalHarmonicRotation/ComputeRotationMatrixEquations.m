clear
close all
clc

pkg load symbolic

N = 2;
nSH = (N + 1)^2;

az = sym('az', [1, 3]);
el = sym('el', [1, 3]);

% Spherical coordinates to Cartesian coordinates
x = cos(el) .* cos(az);
y = cos(el) .* sin(az);
z = sin(el);
cartesian_coords = [x; y; z];

% Define rotation matrices
syms yaw pitch roll real;
R_yaw = [cos(yaw) sin(yaw) sym(0); -sin(yaw) cos(yaw) sym(0); sym(0) sym(0) sym(1)];
R_pitch = [cos(pitch) sym(0) -sin(pitch); sym(0) sym(1) sym(0); sin(pitch) sym(0) cos(pitch)];
R_roll = [sym(1) sym(0) sym(0); sym(0) cos(roll) sin(roll); sym(0) -sin(roll) cos(roll)];

% Compose the full rotation matrix
R_full = R_roll * R_pitch * R_yaw;
disp('Symbolic rotation matrix')
disp(R_full)

% Rotation matrices for full and individual rotations
disp("Calculate spherical harmonic rotation matrices")
rotMatSph_full = simplify(symbolicSphHarmRotMat(R_full,N));
rotMatSph_yaw = simplify(symbolicSphHarmRotMat(R_yaw,N));
rotMatSph_pitch = simplify(symbolicSphHarmRotMat(R_pitch,N));
rotMatSph_roll = simplify(symbolicSphHarmRotMat(R_roll,N));

disp("Yaw rotation matrix")
disp(rotMatSph_yaw)

disp("Pitch rotation matrix")
disp(rotMatSph_pitch)

disp("Roll rotation matrix")
disp(rotMatSph_roll)

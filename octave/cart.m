function [x,y,z] = cart(pol)
    % Convert spherical coordinates to ADM defined cartesian
    
    pol(1:2) = pol(1:2)/180*pi;
    
    x = sin(-pol(1))*cos(pol(2))*pol(3);
    y = cos(-pol(1))*cos(pol(2))*pol(3);
    z = sin(pol(2))*pol(3);
    
end

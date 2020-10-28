function [az,el,r] = pol(cartCoords)
    % Convert from ADM cartesian to spherical coordinates
    
    az = -180/pi*atan2(cartCoords(:,1),cartCoords(:,2));
    el = 180/pi*atan2(cartCoords(:,3),sqrt(cartCoords(:,1).^2 + cartCoords(:,2).^2));
    r = sqrt(cartCoords(:,1).^2 + cartCoords(:,2).^2 + cartCoords(:,3).^2);
    
end

function R = symbolicSphHarmRotMat(Rxyz, L)
%GETSHROTMTX Rotation matrices of real/complex spherical harmonics
%   GETSHROTMTX computes the rotation matrices for real spherical
%   harmonics according to the recursive method of Ivanic and Ruedenberg,
%   as can be found in
%
%       Ivanic, J., Ruedenberg, K. (1996). Rotation Matrices for Real
%       Spherical Harmonics. Direct Determination by Recursion. The Journal
%       of Physical Chemistry, 100(15), 6342?6347.
%
%   and with the corrections of
%
%       Ivanic, J., Ruedenberg, K. (1998). Rotation Matrices for Real
%       Spherical Harmonics. Direct Determination by Recursion Page: Additions
%       and Corrections. Journal of Physical Chemistry A, 102(45), 9099?9100.
%
%   The code implements directly the equations of the above publication and
%   is based on the code, with modifications, found in submision
%   http://www.mathworks.com/matlabcentral/fileexchange/15377-real-valued-spherical-harmonics
%   by Bing Jian
%   and the C++ implementation found at
%   http://mathlib.zfx.info/html_eng/SHRotate_8h-source.html.
%
%   Apart from the real rotation matrices, for which the algorithm is
%   defined, the function returns also the complex SH rotation matrices, by
%   using the transformation matrices from real to complex spherical
%   harmonics. This way bypasses computation of the complex rotation
%   matrices based on explicit formulas of Wigner-D matrices, which would
%   have been much slower and less numerically robust.
%
%   TODO: Recursive algorithm directly for complex SH as found in
%
%       Choi, C. H., Ivanic, J., Gordon, M. S., & Ruedenberg, K. (1999). Rapid
%       and stable determination of rotation matrices between spherical
%       harmonics by direct recursion. The Journal of Chemical Physics,
%       111(19), 8825.
%
%   Inputs:
%
%       L: the maximum band L up to which the rotation matrix is computed
%       Rxyz: the normal 3x3 rotation matrix for the cartesian system
%       basisType: 'real' or 'complex' SH
%
%   Outputs:
%
%       R: the (L+1)^2x(L+1)^2 block diagonal matrix that rotates the frame
%       to the desired orientation
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%   Archontis Politis, 10/06/2015
%   archontis.politis@aalto.fi
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% This file has been modified by Peter Stitt (April 2024) to be compatible
% with Octave's Symbolic package. The original file was released under the
% BSD-3-clause license
%
%Copyright (c) 2015, Archontis Politis All rights reserved.
%
%Redistribution and use in source and binary forms, with or without modification,
% are permitted provided that the following conditions are met:
%
%Redistributions of source code must retain the above copyright notice, this
% list of conditions and the following disclaimer.
%
%Redistributions in binary form must reproduce the above copyright notice, this
% list of conditions and the following disclaimer in the documentation and/or
% other materials provided with the distribution.
%
%Neither the name of Higher-Order-Ambisonics nor the names of its contributors
% may be used to endorse or promote products derived from this software without
% specific prior written permission.
%
%THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
% ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
% WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
% IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
% INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
% BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
% DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
% OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
% OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
% OF THE POSSIBILITY OF SUCH DAMAGE.

if nargin<3
    basisType = 'real';
end
% allocate total rotation matrix
R = sym(zeros((L+1)^2));

% initialize zeroth and first band rotation matrices for recursion
%	Rxyz = [Rxx Rxy Rxz
%           Ryx Ryy Ryz
%           Rzx Rzy Rzz]
%
% zeroth-band (l=0) is invariant to rotation
R(1) = sym(1);

% the first band (l=1) is directly related to the rotation matrix
R_1(-1+2,-1+2) = Rxyz(2,2);
R_1(-1+2, 0+2) = Rxyz(2,3);
R_1(-1+2, 1+2) = Rxyz(2,1);
R_1( 0+2,-1+2) = Rxyz(3,2);
R_1( 0+2, 0+2) = Rxyz(3,3);
R_1( 0+2, 1+2) = Rxyz(3,1);
R_1( 1+2,-1+2) = Rxyz(1,2);
R_1( 1+2, 0+2) = Rxyz(1,3);
R_1( 1+2, 1+2) = Rxyz(1,1);

R(2:4,2:4) = R_1;
R_lm1 = R_1;

% compute rotation matrix of each subsequent band recursively
band_idx = 4;
for l = 2:L
    sym_l = sym(l);
    R_l = sym(zeros(2*l+1));
    for m=-l:l
      sym_m = sym(m);
        for n=-l:l
          sym_n = sym(n);

            % compute u,v,w terms of Eq.8.1 (Table I)
            if (m==0)
              d = sym(1);
            else
              d = sym(0);
            end
            if abs(n)==l
                denom = (sym(2)*sym_l)*(sym(2)*sym_l-sym(1));
            else
                denom = (sym_l*sym_l-sym_n*sym_n);
            end
            u = sqrt((sym_l*sym_l-sym_m*sym_m)/denom);
            v = sqrt((sym(1)+d)*(sym_l+abs(sym_m)-sym(1))*(sym_l+abs(sym_m))/denom)*(sym(1)-sym(2)*d)*sym(1)/sym(2);
            w = sqrt((sym_l-abs(sym_m)-sym(1))*(sym_l-abs(sym_m))/denom)*(sym(1)-d)*(-sym(1)/sym(2));

            % computes Eq.8.1
            if u~=sym(0), u = u*U(sym_l,sym_m,sym_n,R_1,R_lm1); end
            if v~=sym(0), v = v*V(sym_l,sym_m,sym_n,R_1,R_lm1); end
            if w~=sym(0), w = w*W(sym_l,sym_m,sym_n,R_1,R_lm1); end
            R_l(m+l+1,n+l+1) = u + v + w;
        end
    end
    R(band_idx+(1:2*l+1), band_idx+(1:2*l+1)) = R_l;
    R_lm1 = R_l;
    band_idx = band_idx + sym(2)*l+sym(1);
end

end


% functions to compute terms U, V, W of Eq.8.1 (Table II)
function [ret] = U(l,m,n,R_1,R_lm1)

ret = P(0,l,m,n,R_1,R_lm1);

end

function [ret] = V(l,m,n,R_1,R_lm1)

if (m==0)
    p0 = P(1,l,1,n,R_1,R_lm1);
    p1 = P(-1,l,-1,n,R_1,R_lm1);
    ret = p0+p1;
else
    if (m>0)
        if (m==1)
          d = sym(1);
        else
          d = sym(0);
        end
        p0 = P(sym(1),l,m-sym(1),n,R_1,R_lm1);
        p1 = P(-sym(1),l,-m+sym(1),n,R_1,R_lm1);
        ret = p0*sqrt(sym(1)+d) - p1*(sym(1)-d);
    else
        if (m==-1)
          d = sym(1);
        else
          d = sym(0);
        end
        p0 = P(1,l,m+1,n,R_1,R_lm1);
        p1 = P(-1,l,-m-1,n,R_1,R_lm1);
        ret = p0*(1-d) + p1*sqrt(1+d);
    end
end

end

function [ret] = W(l,m,n,R_1,R_lm1)

if (m==0)
    error('should not be called')
else
    if (m>0)
        p0 = P(1,l,m+1,n,R_1,R_lm1);
        p1 = P(-1,l,-m-1,n,R_1,R_lm1);
        ret = p0 + p1;
    else
        p0 = P(sym(1),l,m-sym(1),n,R_1,R_lm1);
        p1 = P(-sym(1),l,-m+sym(1),n,R_1,R_lm1);
        ret = p0 - p1;
    end
end

end

% function to compute term P of U,V,W (Table II)
function [ret] = P(i,l,a,b,R_1,R_lm1)

ri1 = R_1(i+2,1+2);
rim1 = R_1(i+2,-1+2);
ri0 = R_1(i+2,0+2);

if (b==-l)
    ret = ri1*R_lm1(a+l,1) + rim1*R_lm1(a+l, sym(2)*l-sym(1));
else
    if (b==l)
        ret = ri1*R_lm1(a+l,sym(2)*l-sym(1)) - rim1*R_lm1(a+l, sym(1));
    else
        ret = ri0*R_lm1(a+l,b+l);
    end
end

end

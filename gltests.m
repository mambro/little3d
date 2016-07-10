syms n f fov gw gh ratio w h fx fy cx cy X Y Z s real;
% s is the scaling factor of the image plate camera
% r_f r_n is the rage of DepthRange
r_f = 1;
r_n = 0;
% enforce the near/far range (in meters)
%f=100; %m
%n=0.01; %m
a = (f+n)/(n-f);
b = 2*f*n/(n-f);
P=[fov/ratio 0 0 0; 0 fov 0 0; 0 0 a b; 0 0 -1 0]; % not used
qf=  P*[0 0 -f 1]'
qf=simplify(qf/qf(4))
qn=  P*[0 0 -n 1]'
qn=simplify(qn/qn(4))


%TCE = P; % classic Projection
TVN = [gw/2 0 0 gw/2; 0 gh/2 0 gh/2; 0 0 (r_f-r_n)/2 (r_n+r_f)/2; 0 0 0 1];
W = [1 0 0 0; 0 1 0 0; 0 0 -1 0; 0 0 0 1]; % to opengl 
TEW = W; % so we can adapt the projection
Tsc4 = [fx 0 cx 0; 0 fy cy 0; 0 0 f/(f-n) -f*n/(f-n); 0 0 1 0]; % 4D K
Tsc3 = [fx 0 cx ; 0 fy cy ; 0 0 1]; % classic K
TVi = [1 0 0 0; 0 -1 0 gh; 0 0 1 0; 0 0 0 1];
TVif = inv(TVi);

% apply the custom range of opengl using f,n
divw = @(p) p/p(4);
divz = @(p) p/p(3);

gw = s*w;
gh = s*h;
Tscaleview = [s,0,0,0; 0,s,0,0; 0,0,1,0;0,0,0,1];

% in case of different resolution we could have the mapping 
% we need to clarify the Z value
TiN = [gw/2 0 0 gw/2; 0 -gh/2 0 -gh; 0 0 (r_f-r_n)/2 (r_n+r_f)/2; 0 0 0 1];
p = [X Y Z 1]';

% then we can move 
P4 = inv(TVN)*Tscaleview*Tsc4*inv(W);
P4i = inv(TVN)*inv(TVi)*Tscaleview*Tsc4*inv(W);

% verification
qc = divz(Tsc3*p(1:3));
qg = simplify(TVN*divw(P4*TEW*p))
% adjust qc with inverse of physical depth
qc(3) = 1/Z;
qcg = TVi*[qc;1]

simplify(qg-qcg)

%%
%can we test it? YES use the aruco2json info THAT works
q =[];
q.K =[[532.850,0.0,316.880];[0.0,532.450,241.630];[0.0,0.0,1.0]];
q.glp =reshape([1.665156173706055,-0.0,0.0,0.0,0.0,-2.662250061035156,0.0,0.0,0.009749984741210915,0.2081500244140626,-1.002002002002002,-1.0,0.0,-0.0,-0.2002002002002002,0.0],4,4);
q.w = 640;
q.h = 400;
q.nf = [0.1,100];
q.t = [0.01256639789789915,0.03324272111058235,0.4833259284496307];
q.glmv = reshape([-0.05342024937272072,0.8759374022483826,0.4794579446315765,0.0,0.9959275722503662,0.08165632933378220,-0.03821634873747826,0.0,0.07262590527534485,-0.47546386718750,0.8767323493957520,0.0,0.01256639324128628,0.03324271738529205,-0.4833259284496307,1.0],4,4);
q.center = [330.401855468750,280.7149658203125];
q.pose = reshape([[-0.05342030525207520,0.9959275722503662,0.07262593507766724,0.01256639789789915],[0.8759374618530273,0.08165638893842697,-0.4754637479782104,0.03324272111058235],[-0.4794578552246094,0.03821635618805885,-0.8767324090003967,0.4833259284496307],[0.0,0.0,0.0,1.0]],4,4)';


qg = TVN*divw(q.glp*W*p);
qc = divz(q.K*p(1:3));
qc = TVif*[qc;1]
y = double(subs(simplify(qc-qg),{X,Y,Z,w,h,gw,gh,s},{1,2,0.1,q.w,q.h,q.w,q.h,1.0}))


% project origin of modelview to the center (should give q.center) ==
% ASSUME OpenGL flipped image so at the end we need to flip the OpenGL
% viewport
%OK, 2.4 pixel error
pixerror = double(subs(inv(TVi)*TVN*divw(q.glp*q.glmv*[0 0 0 1]'),{w,h,gw,gh},{q.w,q.h,q.w,q.h}))-[q.center,NaN,1]'

% compute our P
%n=q.nf(1);
%f=q.nf(2);
TPFN = [1 0 0 0; 0 1 0 0; 0 0 (f+n)/(n-f) (2*f*n)/(n-f); 0 0 0 1];
P4 = inv(TVN)*TVif*Tsc*inv(W);
% manually fix the Z transformation 
%P4(3,3) = TPFN(3,3);
%P4(3,4) = TPFN(3,4);
P4n = double(subs(P4,{n,f,fx,fy,cx,cy,w,h,s,gw,gh},{0.1,100,q.K(1,1),q.K(2,2),q.K(1,3),q.K(2,3),q.w,q.h,1.0,q.w,q.h}))
q.glp
% EXACT assuming flip of image plate
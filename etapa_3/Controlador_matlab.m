clc;
clear;
close all;

%% CONFIGURACAO VISUAL DOS GRAFICOS

set(0,'DefaultFigureColor','w');
set(0,'DefaultAxesColor','w');
set(0,'DefaultAxesFontSize',12);
set(0,'DefaultAxesLineWidth',1.2);
set(0,'DefaultLineLineWidth',2);

%% PARAMETROS DO SISTEMA

M = 0.892;          % massa do carro [kg]
m = 0.510;          % massa do pendulo [kg]
L = 0.227;          % distancia ao centro de massa [m]
J = 9.72e-3;        % momento de inercia [kg.m^2]
g = 9.81;

% pior caso
b = 0;
d = 0;

%% MATRIZES DO SISTEMA

p = J*(M+m) + M*m*L^2;

A = [0 1 0 0;
     0 0 (m^2*g*L^2)/p 0;
     0 0 0 1;
     0 0 (m*g*L*(M+m))/p 0];

B = [0;
     (J+m*L^2)/p;
     0;
     (m*L)/p];

%% MATRIZES LQR

Q = diag([10 1 500 10]);

R = 1;

%% GANHO LQR

K = lqr(A,B,Q,R);

disp('Ganho K:')
disp(K)

%% SIMULACAO

dt = 0.001;          % passo de simulacao
T  = 10;             % tempo total

t = 0:dt:T;

N = length(t);

%% VETOR DE ESTADOS

x = zeros(4,N);

% condicao inicial
% [posicao, velocidade, angulo, velocidade angular]

x(:,1) = [0;
          0;
          deg2rad(5);
          0];

%% VETOR DE CONTROLE

u = zeros(1,N);

%% SATURACAO DO MOTOR

Fmax = 10;

%% LOOP DE SIMULACAO

for k = 1:N-1

    % controle LQR
    u(k) = -K*x(:,k);

    % saturacao
    if u(k) > Fmax
        u(k) = Fmax;
    end

    if u(k) < -Fmax
        u(k) = -Fmax;
    end

    % dinamica do sistema
    xdot = A*x(:,k) + B*u(k);

    % integracao numerica
    x(:,k+1) = x(:,k) + xdot*dt;

end

%% GRAFICO POSICAO

figure;

plot(t, x(1,:));

grid on;

xlabel('Tempo [s]');
ylabel('Posicao [m]');

title('Posicao do carro');

%% GRAFICO VELOCIDADE

figure;

plot(t, x(2,:));

grid on;

xlabel('Tempo [s]');
ylabel('Velocidade [m/s]');

title('Velocidade do carro');

%% GRAFICO ANGULO

figure;

plot(t, rad2deg(x(3,:)));

grid on;

xlabel('Tempo [s]');
ylabel('Angulo [graus]');

title('Angulo do pendulo');

%% GRAFICO VELOCIDADE ANGULAR

figure;

plot(t, rad2deg(x(4,:)));

grid on;

xlabel('Tempo [s]');
ylabel('Velocidade angular [graus/s]');

title('Velocidade angular do pendulo');

%% GRAFICO CONTROLE

figure;

plot(t, u);

grid on;

xlabel('Tempo [s]');
ylabel('Forca [N]');

title('Sinal de controle');

%% AUTOVALORES

disp('Autovalores em malha fechada:')

disp(eig(A-B*K))


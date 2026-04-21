Etapa 2
#######

.. contents::
   :local:
   :depth: 2


Visão geral
***********

A Etapa 2 consite na estruturação prática do sistema de controle, envolvendo a
 análise e seleção de técnicas de controle  ao problema, 
 bem como o desenvolvimento de uma interface para testes.
  Além disso, são realizados ensaios do sistema de acionamento 
  em malha aberta, permitindo a observação do seu comportamento. Nessa etapa também foi definido o microcontrolador a ser utilizado 
  e realizado  a identificação de parâmetros fundamentais.

Desenvolvimento

***************
****(A primeira etapa desenvolvida consistiu na definição do método de controle a ser empregado no sistema.
Optou-se pela utilização do controle do tipo Linear Quadratic Regulator (LQR).

Embora o LQR exija a modelagem do sistema e maior esforço computacional quando comparado ao controle PID, 
ele se mostrou mais adequado para a aplicação proposta.
 Isso se deve ao fato de o pêndulo invertido ser um sistema naturalmente instável, multivariável 
 e altamente sensível a perturbações, características que dificultam a obtenção de desempenho satisfatório
  por métodos empíricos.

O LQR permite a obtenção de uma lei de controle baseada em modelo, capaz de minimizar uma função de custo que pondera 
simultaneamente o erro de estado e o esforço de controle. Dessa forma, é possível garantir 
maior estabilidade, resposta dinâmica mais previsível.)*****

Em seguida, realizou-se a caracterização do sistema com o objetivo de obter parâmetros físicos  para as etapas subsequentes de modelagem e projeto de controle. Para isso, o robô pêndulo invertido foi desmontado,
para  a medição individual de massa e dimensões de cada componente estrutural. 
Isso possibilitou a estimativa da distribuição de massa, dimensões características e,
 posteriormente, a obtenção de parâmetros relevantes como centro de massa e momentos de inércia aproximados.

Inicialmente, foi medido o peso total do sistema completo, resultando em 1372 g.
 Em seguida, as plataformas  foram removidas e analisadas individualmente, conforme apresentado na Tabela abaixo.

               Peso(g)     Largura(mm)      Comprimento (cm)        altura(mm)
Plataforma 1     371          7,50                24,1                  15,0     
Plataforma 2     58           7,50                15,0                  7,3
Plataforma 3     50           7,50                15,0                  7,4
Plataforma 4     57           7,50                15,0                  15,3


 figure:: img/plataformas.jpg
   :width: 30%
   :align: center

   Figura 2 – Plataformas do robô.   
  
   Fonte: Dos autores (2026).


A Plataforma 1, que suporta a bateria, apresenta altura total de 48,6 mm quando montada.

As quatro hastes de sustentação possuem massa total de 41 g, diâmetro aproximado de 6,4 mm e comprimento de 28,7 cm, 
sendo responsáveis pela  estruturação e espaçamento entre os níveis do sistema.

 figure:: img/hastes.jpg
   :width: 30%
   :align: center

   Figura 2 – Haste presente no robô.   
  
   Fonte: Dos autores (2026).

Adicionalmente, o conjunto de porcas apresenta massa total de 104 g, 
que contribui significativamente para a massa no seu total, embora suas dimensões não sejam relevantes para a modelagem dinâmica.

figure:: img/porcas.jpg
   :width: 30%
   :align: center

   Figura 3 – Porcas.   
  
   Fonte: Dos autores (2026).


O conjunto motor-roda possui massa de 241 g, com diâmetro de roda de 65,4 mm, sendo este parâmetro essencial para
 a relação entre deslocamento linear e rotação do motor.


figure:: img/motores.jpg
   :width: 30%
   :align: center

   Figura 4 – Motores com as rodas.   
  
   Fonte: Dos autores (2026).

A partir desses dados, torna-se possível construir um modelo  aproximado do sistema.

Além disso, foi realizado o levantamento dos componentes necessários para o desenvolvimento do sistema
de controle do pêndulo invertido. Entretanto, como o robô já possui todos os elementos mecânicos e 
de atuação, não houve necessidade de aquisição ou substituição desses componentes.

Dessa forma, a única decisão de hardware relevante nesta etapa foi a escolha do microcontrolador responsável pela 
implementação do controle. Optou-se pela utilização da placa Blackpill (STM32F4), devido à sua capacidade de processamento, 
maior frequência de operação, presença de periféricos como temporizadores de alta resolução, comunicação serial e 
interfaces de entrada/saída, 
além de oferecer melhor desempenho em aplicações de controle em tempo real quando comparada a alternativas mais simples.


Desenvolvimento da interface de testes
teste de acionamento 

Testes
======
1300g peso total com tudo
peso da ultima plataforma  57g  as 3 tem 7.50mm de largura  altura 15.3 mm todos sao 15cm de comprimento 
peso da 3 plataforma 50g altura 7.3
peso da 2 plataforma 58g altura 7,4mm
primeir abase com bateria e ,,371g| 15,0mm altura e 24,1 cm de comprimento

altura com a bateria 48,6mm 

peso de 1 haste x4  41g grosssura? de 6,4mm
comprimento 28,7 cm 
haste 1-39
2-39
39
pesos das porcas 104g

um motor com roda 241g
dois motor com roda 241g

diametro da roda com pneu= 65,4 mm
diametro sem o pneu =51,4mm  
distancia da roda e do motor juntos = 93 


Descrição dos testes/validações realizadas.
a caracterizaçao do sistema as medidas das coisas 

(Outras subseções se necessário)
================================


Referências (links/datasheets/livros)
*************************************

- `nRF Connect SDK <https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.2/nrf/getting_started/modifying.html#configure-application>`_



Etapa 4
#######

.. contents::
   :local:
   :depth: 2


Visão geral
***********

A etapa 4 consiste na fabricação da PCB e sua integração ao robô, junto com os ajustes e refinamentos necessários no software de controle desenvolvido na etapa anterior. Além disso,  é realizado os testes de estabilidade para avaliar o desempenho do sistema, identificar falhas de funcionamento e implementar as correções  para que o robô opere conforme o comportamento esperado ao final.
A etapa 4 é a etapa mais curta, uma vez que essa se  refere apenas aos ajustes de partes ja realizadas nas etapas anteriores 


Desenvolvimento
***************

A primeira parte do desenvolvimento da etapa 4 começou com  a melhoria da placa que já estava instalada no robô. Essa placa era antiga e, como foi necessário retirar e adicionar alguns cabos durante os testes, percebeu-se que as soldas dos cabos estavam bastante danificadas. Isso fazia com que os cabos se desconectassem com frequência e até identificar que o problema era esse, demorava bastante tempo.

Esse problema acontecia principalmente porque, durante os testes do sistema de controle, o robô precisava ser colocado em funcionamento várias vezes e  como estávamos testando acabava caindo durante os ensaios. Com o impacto das quedas e a movimentação dos cabos, as conexões acabavam se soltando, prejudicando os testes e dificultando a identificação correta dos problemas do sistema.

Para resolver isso, foram adicionados conectores na placa. Com essa alteração, as ligações ficaram mais organizadas, a montagem ficou mais limpa e a chance de desconexão diminuiu. Além disso, o uso de conectores facilita a manutenção, já que caso seja necessário retirar a placa do robô eles poder ser removidos e recolocados com mais segurança.

Durante essa etapa também foi identificado outro problema relacionado aos encoders dos motores, que não estavam funcionando. Inicialmente, acreditava-se que o erro poderia estar no código, pois na primeira vez que testamos eles estavam funcionando. Porém, ao analisar a placa e comparar as conexões com o datasheet do fabricante, verificou-se que a pinagem indicada não correspondia ao comportamento real observado.

Isso ficou mais claro ao testar outro motor, no qual foi possível observar o acionamento de uma luz indicativa que não acendia no motor instalado no robô. A partir disso, as conexões foram  alteradas. Após a correção da ligação dos pinos, os encoders passaram a funcionar, mostrando que o problema não estava no software, mas sim na conexão elétrica feita com base em uma pinagem incorreta.

 Figura 1 – Conexões antigas.
.. figure:: img/.png
   :width: 30%
   :align: center

 Figura 2 – Conexões Novas.
.. figure:: img/.png
   :width: 30%
   :align: center


Testes
======

Descrição dos testes/validações realizadas.


(Outras subseções se necessário)
================================


Referências (links/datasheets/livros)
*************************************

- `nRF Connect SDK <https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.2/nrf/getting_started/modifying.html#configure-application>`_



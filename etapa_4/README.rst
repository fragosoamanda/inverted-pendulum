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

Após a melhoria da placa já existente no robô, iria ser iniciada a confecção da nova PCB, considerando as modificações realizadas no KiCad a partir das orientações dos professores. Isso porque, durante os testes, percebeu-se que a montagem em protoboard estava dificultando bastante a organização do sistema.

Como eram utilizados muitos cabos, as conexões  ficavam mais frágeis e confusas. Isso atrapalhava principalmente a etapa de testes, que é uma das partes mais importantes do desenvolvimento, pois é nela que se verificava se o robô esta respondendo corretamente se o sistema de controle esta funcionando como esperado.

Por esse motivo, foi sugerida a utilização de uma placa perfurada de fenolite, também conhecida como  placa padrão ilhada. Essa placa já possui furos para a fixação dos componentes e pode ser cortada e ajustada para caber melhor dentro do robô. Diferente de uma PCB típica, na placa ilhada as trilhas são feitas manualmente,  por meio de soldas e pequenos fios de ligação.

Figura3 – Placa Padrão Ilhada.
.. figure:: img/.png
   :width: 30%
   :align: center

Como o esquemático da placa já havia sido desenvolvido no KiCad, a montagem nessa placa foi feita seguindo as conexões já definidas anteriormente. Dessa forma, foi necessário apenas analisar o esquemático e reproduzir manualmente as ligações entre os componentes.

Essa solução foi escolhida por ser mais simples e rápida, que era o que o grupo precisava. Como o objetivo principal naquele momento era validar o funcionamento do robô, principalmente a parte de controle, a fabricação de uma PCB definitiva poderia atrasar o desenvolvimento. A placa perfurada permitiu montar um circuito mais organizado do que a protoboard, mas sem exigir todo o processo de fabricação de uma placa final.

Além disso, foi adicionado um conector do tipo fita, com o objetivo de facilitar ainda mais as conexões  que vinham da placa já existente no robô. Com isso, a integração entre as placas ficou mais organizada e confiável, reduzindo a quantidade de fios soltos e facilitando, caso necessário, ajustes durante os testes.

Figura 4 – Placa com os componentes no robô.
.. figure:: img/.png
   :width: 30%
   :align: center

Figura 5 – Conector fita utilizado.
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

SHENZHEN JINSHUNLAITE MOTOR CO., LTD. **37mm Round Spur Gear Motor**. [S. l.]: Aslong Motor, 2021. Disponível em: https://www.aslongdcmotor.com/photo/aslongdcmotor/document/26547/37mm%20Round%20Spur%20Gear%20Motor_PDF00.pdf. Acesso em: 25 jun. 2026.


CASA DA ROBÓTICA. **Placa Fibra Ilhada 10x10 cm Padrão PCB Perfurada**. [S. l.], [s. d.]. Disponível em: https://www.casadarobotica.com/placa-fenolite-ilhada-10x10-cm-padrao-pcb-perfurada-arduino. Acesso em: 25 jun. 2026.




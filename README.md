Os arquivos neste repositório contém os seguintes códigos:
- "gerador_movel.ino": código enviado ao ESP32 associado ao sistema de geração com rastreamento. O código contém a aquisição e envio de dados, bem como a lógica de movimentação do protótipo.
- "gerador_fixo.ino": código enviado ao ESP32 associado ao sistema de geração fixo. O código contém a aquisição e envio de dados.
- "calibracao_LDR.ino": código desenvolvido na Arduino IDE para captar as leituras dos LDRs durante o procedimento conduzido em câmara escura.
- "calibracao_LDRs.m": código desenvolvido no MATLAB para obtenção das equações de calibração dos sensores LDR em câmara escura.
- "dispersao_LDRs.m": código desenvolvido no MATLAB para cálculo de disperão entre sensores LDR antes e depois da calibração em câmara escura.
- "teste_gerador_fixo.ino": código desenvolvido na Arduino IDE para teste dos sensores do sistema fixo montado em protoboard (fase de validação).
- "teste_gerador_movel.ino": código desenvolvido na Arduino IDE para teste de sensores e atuadores do sistema móvel montado em protoboard (fase de validação).

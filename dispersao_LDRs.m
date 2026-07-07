% Vetor de luminosidade de referencia (luximetro)
lux = [474, 561, 672, 755, 834, 916, 982, 1076, 1143, 1221, ...
       1422, 1631, 1831, 2027, 2201, 2374, 2555, 2750, 2924, ...
       3086, 3265, 3409, 3590, 3764, 3896, 4075];

% Tensoes medias por LDR (V)
V1 = [2.170, 2.260, 2.320, 2.360, 2.400, 2.440, 2.473, 2.510, ...
      2.540, 2.560, 2.620, 2.663, 2.710, 2.743, 2.770, 2.803, ...
      2.827, 2.850, 2.870, 2.877, 2.900, 2.920, 2.933, 2.933, ...
      2.950, 2.970];

V2 = [2.410, 2.490, 2.547, 2.590, 2.620, 2.677, 2.693, 2.720, ...
      2.750, 2.763, 2.833, 2.860, 2.893, 2.930, 2.957, 2.980, ...
      3.000, 3.020, 3.030, 3.070, 3.057, 3.070, 3.087, 3.090, ...
      3.107, 3.093];

V3 = [1.990, 2.090, 2.170, 2.220, 2.260, 2.300, 2.350, 2.380, ...
      2.410, 2.437, 2.510, 2.560, 2.613, 2.650, 2.690, 2.713, ...
      2.740, 2.770, 2.793, 2.820, 2.833, 2.853, 2.870, 2.880, ...
      2.897, 2.910];

V4 = [2.307, 2.387, 2.447, 2.490, 2.527, 2.570, 2.607, 2.630, ...
      2.660, 2.677, 2.740, 2.780, 2.820, 2.850, 2.880, 2.903, ...
      2.930, 2.950, 2.970, 2.980, 3.000, 3.010, 3.027, 3.033, ...
      3.043, 3.060];

% ANTES DA CALIBRAGEM
% Dispersao baseada nas tensoes brutas

% Media das tensoes brutas entre os 4 LDRs
V_media = (V1 + V2 + V3 + V4) / 4;

% Diferenca de cada LDR em relacao a media 
dV1 = (V1 - V_media) * 1000;
dV2 = (V2 - V_media) * 1000;
dV3 = (V3 - V_media) * 1000;
dV4 = (V4 - V_media) * 1000;

% Conversao dos desvios de tensao para percentual da faixa total
faixa_V = (max([V1 V2 V3 V4]) - min([V1 V2 V3 V4])) * 1000;
dV1_pct = (dV1 / faixa_V) * 100;
dV2_pct = (dV2 / faixa_V) * 100;
dV3_pct = (dV3 / faixa_V) * 100;
dV4_pct = (dV4 / faixa_V) * 100;

% DEPOIS DA CALIBRAGEM
% Dispersao baseada nos valores de lux estimados pelas equacoes
L1 = 0.7071 .* V1.^7.9288;
L2 = 0.0493 .* V2.^9.9301;
L3 = 3.3803 .* V3.^6.6034;
L4 = 0.1976 .* V4.^8.8516;

% Media dos lux estimados entre os 4 LDRs
L_media = (L1 + L2 + L3 + L4) / 4;

% Diferenca de cada LDR em relacao a media (em lux)
dL1 = L1 - L_media;
dL2 = L2 - L_media;
dL3 = L3 - L_media;
dL4 = L4 - L_media;

% Conversao dos desvios de lux para percentual da faixa total
faixa_L = max(lux) - min(lux);
dL1_pct = (dL1 / faixa_L) * 100;
dL2_pct = (dL2 / faixa_L) * 100;
dL3_pct = (dL3 / faixa_L) * 100;
dL4_pct = (dL4 / faixa_L) * 100;

% GRAFICO UNICO COM DOIS PAINEIS
cores = {[0.00 0.45 0.74], [0.85 0.33 0.10], [0.47 0.67 0.19], [0.49 0.18 0.56]};
marcadores = {'o', 's', '^', 'd'};
nomes = {'LDR1 (GPIO 34)', 'LDR2 (GPIO 35)', 'LDR3 (GPIO 36 / VP)', 'LDR4 (GPIO 39 / VN)'};
figure('Position', [100, 100, 1200, 480]);

% Painel esquerdo - Antes da calibragem
subplot(1, 2, 1);
hold on;
dados_antes = {dV1_pct, dV2_pct, dV3_pct, dV4_pct};
for i = 1:4
    plot(lux, dados_antes{i}, [marcadores{i} '-'], 'Color', cores{i}, ...
        'LineWidth', 1.5, 'MarkerFaceColor', cores{i}, 'DisplayName', nomes{i});
end
yline(0, '--k', 'Media dos LDRs', 'LineWidth', 1.0, ...
    'LabelHorizontalAlignment', 'left', 'FontSize', 9);
xlabel('Luminosidade de referencia (lux)', 'FontSize', 11);
ylabel('Desvio em relacao a media (% da faixa)', 'FontSize', 11);
title('Antes da Calibragem', 'FontSize', 12);
legend('Location', 'best', 'FontSize', 9);
grid on;
hold off;

% Painel direito - Depois da calibragem
subplot(1, 2, 2);
hold on;
dados_depois = {dL1_pct, dL2_pct, dL3_pct, dL4_pct};
for i = 1:4
    plot(lux, dados_depois{i}, [marcadores{i} '-'], 'Color', cores{i}, ...
        'LineWidth', 1.5, 'MarkerFaceColor', cores{i}, 'DisplayName', nomes{i});
end
yline(0, '--k', 'Media dos LDRs', 'LineWidth', 1.0, ...
    'LabelHorizontalAlignment', 'left', 'FontSize', 9);
xlabel('Luminosidade de referencia (lux)', 'FontSize', 11);
ylabel('Desvio em relacao a media (% da faixa)', 'FontSize', 11);
title('Depois da Calibragem', 'FontSize', 12);
legend('Location', 'best', 'FontSize', 9);
grid on;
hold off;
sgtitle('Dispersao entre LDRs do Sistema', 'FontSize', 13, 'FontWeight', 'bold');

% DESVIO PADRAO ao longo da faixa - resumo quantitativo
fprintf('\n=== Desvio padrao medio entre LDRs ===\n');
std_antes_pct = mean(std([dV1_pct; dV2_pct; dV3_pct; dV4_pct], 0, 1));
std_depois_pct = mean(std([dL1_pct; dL2_pct; dL3_pct; dL4_pct], 0, 1));
fprintf('Antes da calibragem:  %.2f %% da faixa total\n', std_antes_pct);
fprintf('Depois da calibragem: %.2f %% da faixa total\n', std_depois_pct);c
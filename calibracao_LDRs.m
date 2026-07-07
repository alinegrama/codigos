% Vetor de luminosidade (lux) - referencia do luximetro (celular)
lux = [474, 561, 672, 755, 834, 916, 982, 1076, 1143, 1221, ...
       1422, 1631, 1831, 2027, 2201, 2374, 2555, 2750, 2924, ...
       3086, 3265, 3409, 3590, 3764, 3896, 4075];

% Vetores de tensao media (V) - media das 3 leituras consecutivas
% GPIO 34
V1 = [2.170, 2.260, 2.320, 2.360, 2.400, 2.440, 2.473, 2.510, ...
      2.540, 2.560, 2.620, 2.663, 2.710, 2.743, 2.770, 2.803, ...
      2.827, 2.850, 2.870, 2.877, 2.900, 2.920, 2.933, 2.933, ...
      2.950, 2.970];
% GPIO 35
V2 = [2.410, 2.490, 2.547, 2.590, 2.620, 2.677, 2.693, 2.720, ...
      2.750, 2.763, 2.833, 2.860, 2.893, 2.930, 2.957, 2.980, ...
      3.000, 3.020, 3.030, 3.070, 3.057, 3.070, 3.087, 3.090, ...
      3.107, 3.093];
% GPIO 36 (VP)
V3 = [1.990, 2.090, 2.170, 2.220, 2.260, 2.300, 2.350, 2.380, ...
      2.410, 2.437, 2.510, 2.560, 2.613, 2.650, 2.690, 2.713, ...
      2.740, 2.770, 2.793, 2.820, 2.833, 2.853, 2.870, 2.880, ...
      2.897, 2.910];
% GPIO 39 (VN)
V4 = [2.307, 2.387, 2.447, 2.490, 2.527, 2.570, 2.607, 2.630, ...
      2.660, 2.677, 2.740, 2.780, 2.820, 2.850, 2.880, 2.903, ...
      2.930, 2.950, 2.970, 2.980, 3.000, 3.010, 3.027, 3.033, ...
      3.043, 3.060];

% Curve Fitting Tool
fprintf('Abrindo Curve Fitting Tool para LDR1 (GPIO 34)...\n');
cftool(V1, lux);
input('Pressione ENTER para continuar para o LDR2...');

fprintf('Abrindo Curve Fitting Tool para LDR2 (GPIO 35)...\n');
cftool(V2, lux);
input('Pressione ENTER para continuar para o LDR3...');

fprintf('Abrindo Curve Fitting Tool para LDR3 (GPIO 36 / VP)...\n');
cftool(V3, lux);
input('Pressione ENTER para continuar para o LDR4...');

fprintf('Abrindo Curve Fitting Tool para LDR4 (GPIO 39 / VN)...\n');
cftool(V4, lux);
input('Pressione ENTER para gerar o grafico comparativo...');

% GRAFICO COMPARATIVO
figure;
hold on;
plot(V1, lux, 'o-', 'Color', [0.00 0.45 0.74], 'LineWidth', 1.5, ...
    'MarkerFaceColor', [0.00 0.45 0.74], 'DisplayName', 'LDR1 (GPIO 34)');
plot(V2, lux, 's-', 'Color', [0.85 0.33 0.10], 'LineWidth', 1.5, ...
    'MarkerFaceColor', [0.85 0.33 0.10], 'DisplayName', 'LDR2 (GPIO 35)');
plot(V3, lux, '^-', 'Color', [0.47 0.67 0.19], 'LineWidth', 1.5, ...
    'MarkerFaceColor', [0.47 0.67 0.19], 'DisplayName', 'LDR3 (GPIO 36 / VP)');
plot(V4, lux, 'd-', 'Color', [0.49 0.18 0.56], 'LineWidth', 1.5, ...
    'MarkerFaceColor', [0.49 0.18 0.56], 'DisplayName', 'LDR4 (GPIO 39 / VN)');
xlabel('Tensao (V)', 'FontSize', 12);
ylabel('Luminosidade (lux)', 'FontSize', 12);
title('Comportamento dos LDRs do Sistema', 'FontSize', 13);
legend('Location', 'northwest', 'FontSize', 10);
grid on;
hold off;
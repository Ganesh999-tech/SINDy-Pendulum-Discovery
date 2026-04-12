%% --- STEP 1: LOAD THE DATA ---
[file, path] = uigetfile('*.csv', 'Select your phyphox Gyroscope CSV file');
if isequal(file,0), error('User cancelled.'); end
fullPath = fullfile(path, file);

opts = detectImportOptions(fullPath);
opts.VariableNamingRule = 'preserve';
data = readtable(fullPath, opts); 

%% --- STEP 2: FIND COLUMNS ---
allNames = data.Properties.VariableNames;
tIdx = find(contains(lower(allNames), 'time'), 1);
wIdx = find(contains(lower(allNames), 'gyroscope x'), 1);

if isempty(tIdx) || isempty(wIdx), error('Columns not found!'); end

t = data{:, tIdx};         
wx = data{:, wIdx};       

%% --- STEP 3: PREPARE PHYSICS ---
% 1. Clean the release wobble
start_time = 15; % Cut first 15s for even cleaner data
clean_idx = t > start_time;
t_pure = t(clean_idx) - t(find(clean_idx,1)); 
wx_pure = wx(clean_idx);

% 2. Zero-offset (Calibration)
wx_pure = wx_pure - mean(wx_pure(end-100:end)); 

% 3. Calculate Theta and Acceleration
% We use 'detrend' to prevent the integration drift
theta = detrend(cumtrapz(t_pure, wx_pure));
dt = mean(diff(t_pure));
wx_dot = gradient(wx_pure, dt);

%% --- STEP 4: SINDy MODELING ---
% Library: [Bias, Velocity(Friction), Gravity, Velocity^2(Drag)]
Library = [ones(size(wx_pure)), wx_pure, sin(theta), abs(wx_pure).*wx_pure];

% Solver
Coefficients = Library \ wx_dot; 

% SPARSITY THRESHOLD: Lowered to 0.005 to catch small friction terms
threshold = 0.005; 
Coefficients(abs(Coefficients) < threshold) = 0;

%% --- STEP 5: THE DISCOVERY REPORT ---
L_measured = 0.43; 
g_theory = 9.81 / L_measured; 

% Math Check: We take absolute values for the comparison
discovered_g_L = abs(Coefficients(3)); 
error_pct = abs(discovered_g_L - g_theory) / g_theory * 100;

% Physics Insight: Calculate Effective Length
% Since g/L = discovered_val, then L_eff = g / discovered_val
L_effective = 9.81 / discovered_g_L;

fprintf('\n====================================\n');
fprintf('     SINDy PHYSICAL DISCOVERY       \n');
fprintf('====================================\n');
fprintf('Friction (Linear):      %.4f\n', Coefficients(2));
fprintf('Gravity (sin theta):    %.4f\n', Coefficients(3));
fprintf('Air Drag (Quadratic):   %.4f\n', Coefficients(4));
fprintf('------------------------------------\n');
fprintf('Textbook Expectation:   %.2f\n', g_theory);
fprintf('Measured Error:         %.2f%%\n', error_pct);
fprintf('Effective Pendulum L:   %.2f cm (Measured: 41cm)\n', L_effective * 100);
fprintf('====================================\n');

%% --- STEP 6: VISUAL VERIFICATION ---
figure('Name', 'SINDy Pendulum Validation');
subplot(2,1,1);
plot(t_pure, wx_pure, 'LineWidth', 1);
title('Sensor Data: Angular Velocity (\omega)'); ylabel('rad/s'); grid on;

subplot(2,1,2);
plot(t_pure, wx_dot, 'r', 'DisplayName', 'Measured Acceleration'); hold on;
plot(t_pure, Library * Coefficients, 'k--', 'LineWidth', 1.5, 'DisplayName', 'SINDy Model');
title('Model Verification: Measured vs. SINDy Predicted'); 
ylabel('rad/s^2'); xlabel('Time (s)'); legend; grid on;
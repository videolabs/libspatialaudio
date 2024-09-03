% Plot the shelf filter magnitudes used for psychoacoustic optimisation
%
% Author: Peter Stitt
clear
close all
clc

F_print = 1;

pkg load signal

figHeight = 0.26;
figWidth = 0.23;
figHorGap = 0.005;
figVertGap = 0.05;

load('filterIRs.mat');
Ns = 100*size(optFilts_o1,1);
f = (0:(Ns-1))/Ns*fs;


for iOrder = 1:3
  ifig = 4*(iOrder-1) + 1;
  if iOrder == 1
    irs = optFilts_o1;
  elseif iOrder == 2
    irs = optFilts_o2;
  elseif
    irs = optFilts_o3;
  endif

  for n = 0:iOrder
    irCh = irs(:,n + 1);

    figure(1)
    subplot(3,4,ifig)
    semilogx(f,mag2db(abs(fft(irCh,Ns))),'LineWidth',4);
    [0.05 + 0.25 * n, 0.95 - (iOrder-1)*0.2, 0.2, 0.3]
    set(gca, 'Position', [0.05 + (figWidth + figHorGap) * n, (1-figVertGap-figHeight) - (iOrder-1)*(figHeight + figVertGap), figWidth, figHeight])
    set(gca, 'FontSize',15)
    set(gca, 'LineWidth',2)
    title(['N = ' num2str(iOrder) ', n = ' num2str(n)])
    xlim([20,20000])
    ylim([-6,6])
    grid on
    if ifig == 4*(iOrder-1) + 1
      ylabel('Magnitude (dB)')
    else
      set(gca,'YTickLabel',[]);
    end
    if iOrder == 3
      xlabel('Frequency (Hz)')
    else
      set(gca,'XTickLabel',[]);
    end

    ifig = ifig + 1;
  end

end

if F_print == 1
  % Set paper position to auto
  set(gcf, 'PaperPositionMode', 'auto');
  % Adjust figure size and axes position
  set(gcf, 'Units', 'inches');
  set(gcf, 'Position', [0, 0, 4, 2]*5); % Adjust the size as needed
  axPos = get(gca, 'Position');

  % Save the figure
  print('shelf_filters.png', '-dpng','-r300');
end


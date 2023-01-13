library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity pulsadorRele is
    Port ( 
        clk     : in    std_logic;
        stop    : in    std_logic;
        pulsador: in    std_logic; -- cuando se presiona es 0
        i2c_data: in    std_logic;
        rele    : inout std_logic
    );
end pulsadorRele;

architecture Behavioral of pulsadorRele is 
    signal counter1 : integer   := 0;
    signal counter2 : integer   := 0;
    signal counterClk: integer  := 0;
    signal num_press: integer   := 0;
    signal t_5s: integer := 0;
    signal t_30s: integer := 0;
    signal pare: std_logic;

begin
    process(clk)
    begin
        if(clk'event and clk='1') then
            
            if (stop='1') then
                pare <= '1';
                rele <= i2c_data;
            end if;
            
            if (pulsador='0') then
                if (counter1 = 1000000) then
                    if (rele = '0') then
                        counterClk <= 1;
                        num_press <= num_press + 1;
                        pare <= '0';
                    else
                        rele <= '0';
                        counterClk <= 0;
                        num_press <= 0;
                        t_5s <= 0;
                        t_30s <= 0;
                    end if;
                end if;
                counter1 <= counter1 + 1;
            else
                counter1 <= 0;
                if(counterClk > 50000000) then -- Ventana de tiempo pulsador 0.5s
                    if(num_press = 1) then
                        t_5s <= 0;
                        t_30s <= 0;
                    elsif (num_press = 2 ) then
                        t_5s <= 1;  --1*5=5s
                        t_30s <= 0;
                    elsif (num_press = 3) then
                        t_5s <= 1;
                        t_30s <= 6; --6*5=30s
                    end if;
                    rele <= '1';
                    counterClk <= 0;
                    num_press <= 0;
                elsif (counterClk > 0) then
                    counterClk <= counterClk + 1;
                end if;
            end if;
            
            if(t_5s > 500000000) then
                if(t_30s <= 1) then
                    rele <= '0';
                    t_5s <= 0;
                else
                    t_5s <= 1;
                    t_30s <= t_30s - 1; 
                end if;
            elsif t_5s > 0 and pare = '0' then
                t_5s <= t_5s + 1;
            end if;
            
        end if;
    end process;
end Behavioral;

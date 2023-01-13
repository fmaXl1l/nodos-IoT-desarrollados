library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity i2c_leds is
    port(
        --I2C
        scl     : inout std_logic;
        sda     : inout std_logic;
        clk     : in    std_logic;
        rst     : in    std_logic;
        --Pulsadores y reles
        pulsadores: in std_logic_vector(7 downto 0); -- cuando se presiona es 0
        reles: inout std_logic_vector(7 downto 0);
        
        --ESP conexion directa
        inEsp: in std_logic_vector(1 downto 0);
        outEsp: out std_logic_vector(1 downto 0);
        pulsadorEsp: in std_logic_vector(1 downto 0);
        releEsp: inout std_logic_vector(1 downto 0); 
        dataReady: in std_logic
    );
end entity;

architecture RTL of i2c_leds is  
    signal read_req         : std_logic;
    signal data_to_master   : std_logic_vector(7 downto 0);
    signal data_valid       : std_logic;
    signal data_from_master : std_logic_vector(7 downto 0);
    
    signal i2c_data : std_logic_vector(7 downto 0);
    signal stop : std_logic_vector(7 downto 0);
    signal stopEsp : std_logic_vector(1 downto 0);

begin        
            
    i2c_slave0 : entity work.i2c_slave(arch) port map(scl,sda,clk,rst,read_req,data_to_master,data_valid,data_from_master);
    p0: entity work.pulsadorRele(Behavioral) port map(clk, stop(0), pulsadores(0), i2c_data(0), reles(0));
    p1: entity work.pulsadorRele(Behavioral) port map(clk, stop(1), pulsadores(1), i2c_data(1), reles(1));
    p2: entity work.pulsadorRele(Behavioral) port map(clk, stop(2), pulsadores(2), i2c_data(2), reles(2));
    p3: entity work.pulsadorRele(Behavioral) port map(clk, stop(3), pulsadores(3), i2c_data(3), reles(3));
    p4: entity work.pulsadorRele(Behavioral) port map(clk, stop(4), pulsadores(4), i2c_data(4), reles(4));
    p5: entity work.pulsadorRele(Behavioral) port map(clk, stop(5), pulsadores(5), i2c_data(5), reles(5));
    p6: entity work.pulsadorRele(Behavioral) port map(clk, stop(6), pulsadores(6), i2c_data(6), reles(6));
    p7: entity work.pulsadorRele(Behavioral) port map(clk, stop(7), pulsadores(7), i2c_data(7), reles(7));
    p8: entity work.pulsadorRele(Behavioral) port map(clk, stopEsp(0), pulsadorEsp(0), inEsp(0), releEsp(0));
    p9: entity work.pulsadorRele(Behavioral) port map(clk, stopEsp(1), pulsadorEsp(1), inEsp(1), releEsp(1));

    process(clk)
    begin
        if(clk'event and clk='1') then
            if (data_valid='1') then
                stop <= reles xor data_from_master;
                i2c_data <= data_from_master;
            else
                stop <= "00000000";
            end if;
            if (dataReady = '1') then
                stopEsp <= releEsp xor inEsp;
            else
                stopEsp <= "00";               
            end if;
        end if;
    end process;
 
    outEsp <= releEsp;
    data_to_master <= reles;
end architecture;

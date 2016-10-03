library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.MYPROC.all;

ENTITY VGASYNC IS
  PORT(
    PXLCLK: IN STD_LOGIC; 
    MEMCLK: IN STD_LOGIC; 
    HSYNC,VSYNC: OUT STD_LOGIC;
    R : OUT STD_LOGIC_VECTOR(4 downto 0);
    G : OUT STD_LOGIC_VECTOR(5 downto 0);
    B : OUT STD_LOGIC_VECTOR(4 downto 0);
    MEM_ADDR : OUT STD_LOGIC_VECTOR(31 downto 0);
    MEM_DATA : IN STD_LOGIC_VECTOR(31 downto 0);
    MEM_RST : OUT STD_LOGIC
  );
END VGASYNC;

ARCHITECTURE MAIN of VGASYNC IS


-- Horizontal timing
constant hva : integer := 640; -- Visible area
constant hfp : integer :=  16; -- Front porch
constant hsp : integer :=  96; -- Sync pulse
constant hbp : integer :=  48; -- Back porch

-- Vertical timing
constant vva : integer := 480; -- Visible area
constant vfp : integer :=  10; -- Front porch
constant vsp : integer :=   2; -- Sync pulse
constant vbp : integer :=  33; -- Back porch


constant init_address : unsigned :=X"40000000";


signal address : unsigned := init_address;

signal HPOS: integer range 0 to (hva+hfp+hsp+hbp):=0;
signal VPOS: integer range 0 to (vva+vfp+vsp+vbp):=0;

signal GRID_DRAW_FLAG: STD_LOGIC := '0';
signal SHAPE_DRAW_FLAG: STD_LOGIC := '0';

signal GRID_DRAW_RGB: std_logic_vector(2 downto 0) := "000";
signal SHAPE_DRAW_RGB: std_logic_vector(2 downto 0) := "000";

signal GRID_ON: std_logic := '0';

signal SHAPES: shapes_arr;

signal SHOULD_READ: STD_LOGIC:='1';
signal MEM_INDX: integer range 0 to N_MAX_SHAPES:=0;
signal MEM_CYCLES: integer range 0 to 2:=0;



BEGIN


MEM_ADDR <= std_logic_vector(address);

DRAW_GRID(HPOS, VPOS, GRID_DRAW_RGB, GRID_DRAW_FLAG);

DRAW_SHAPES(HPOS, VPOS, SHAPES, SHAPE_DRAW_RGB, SHAPE_DRAW_FLAG);


process(MEMCLK)
begin
  if rising_edge(MEMCLK) and SHOULD_READ = '1' then

  -- wait two cycles;

  if MEM_CYCLES < 2 then

    MEM_CYCLES <= MEM_CYCLES +1;

  else

    MEM_CYCLES <= 0;
    address <= address + 4;

    if MEM_INDX > N_MAX_SHAPES -1 then

      MEM_INDX <= 0;
      SHOULD_READ <= '0';
      MEM_RST <= '1';
      address <= unsigned(init_address);

    else

      MEM_INDX <= MEM_INDX +1;

    end if;
    

    if MEM_INDX = 0 then

      GRID_ON <= MEM_DATA(0);

    elsif (MEM_INDX > 0 and MEM_INDX <= N_MAX_SHAPES) then

      SHAPES(MEM_INDX-1) <= MEM_DATA; 

    end if;
    
  end if;

  end if;
end process;



process(PXLCLK)
begin
if rising_edge(PXLCLK) then
  
---------------- SYNC ----------------
  if HPOS < (hva+hfp+hsp+hbp) then
    HPOS <= HPOS + 1;
  else
    HPOS <= 0;
    if VPOS < (vva+vfp+vsp+vbp) then
      VPOS <= VPOS + 1;
    else
      VPOS <= 0;
    end if;
  end if;
  
  if HPOS > (hva+hfp) and HPOS < (hva+hfp+hsp) then
    HSYNC <= '0';
  else
    HSYNC <= '1';
  end if;
  
  if VPOS > (vva+vfp) and VPOS < (vva+vfp+vsp) then
    VSYNC <= '0';
  else
    VSYNC <= '1';
  end if;
--------------------------------------



  if (HPOS = 0 and VPOS = 0) then -- Read from memory
    MEM_RST <= '0';
    SHOULD_READ <= '1';
  end if ;


  if (HPOS > hva) or (VPOS > vva) then -- Outside the visible area.

    -- Reset vars.
    R<=(OTHERS=>'0');
    G<=(OTHERS=>'0');
    B<=(OTHERS=>'0');

  else -- Inside the visible area.

    if (GRID_DRAW_FLAG = '1' and GRID_ON = '1') then

      R<=(OTHERS=>GRID_DRAW_RGB(0));
      G<=(OTHERS=>GRID_DRAW_RGB(1));
      B<=(OTHERS=>GRID_DRAW_RGB(2));  

    elsif (SHAPE_DRAW_FLAG = '1') then

      R<=(OTHERS=>SHAPE_DRAW_RGB(0));
      G<=(OTHERS=>SHAPE_DRAW_RGB(1));
      B<=(OTHERS=>SHAPE_DRAW_RGB(2));

    else -- Background

      R<=(OTHERS=>'0');
      G<=(OTHERS=>'0');
      B<=(OTHERS=>'0');

    end if;

  end if;

end if;
end process;


END MAIN;
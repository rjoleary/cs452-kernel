ENTRY (main)

MEMORY
{
ram : ORIGIN = 0x00218000, LENGTH = 32M /* This line was edited to match RedBoot's value */
rom : ORIGIN = 0x60000000, LENGTH = 8M
}

SECTIONS
{
data : /* Initialized data. */
{
_DataStart = . ;
*(.data)
_DataEnd = . ;
} >ram

bss : /* Uninitialized data. */
{
_BssStart = . ;
*(.bss)
userStart = . ;
*(user_stacks)
userEnd = . ;
_BssEnd = . ;
} >ram

text : /* The actual instructions. */
{
textStart = . ;
*(.text)
*(.text.*)
*(.got)
*(.got.plt)
*(.rodata)
*(.glue_7)
*(.glue_7t)
*(.rodata.str1.4)
textEnd = . ;
} >ram
}

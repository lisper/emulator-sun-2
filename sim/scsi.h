/* */

enum {
  PHASE_BUS_FREE = 0,
  PHASE_ARBITRATION,
  PHASE_SELECTION,
  PHASE_RESELECTION,
  PHASE_DATA_OUT,
  PHASE_DATA_IN,
  PHASE_COMMAND,
  PHASE_STATUS,
  PHASE_MESSAGE_OUT,
  PHASE_MESSAGE_IN,
};

#define SCSI_BUS_RST	0x200
#define SCSI_BUS_BSY	0x100

#define SCSI_BUS_ATN	0x040
#define SCSI_BUS_ACK	0x020
#define SCSI_BUS_SEL	0x010

#define SCSI_BUS_MSG	0x008
#define SCSI_BUS_CD	0x004
#define SCSI_BUS_REQ	0x002
#define SCSI_BUS_IO	0x001

int scsi_set_disk_image(int unit, char *fname);
int scsi_set_tape_image(int unit, int fileno, char *fname);
unsigned int scsi_read_cmd_byte(void);
void scsi_write_cmd_byte(int value);

int _scsi_next_file(int unit);
void _scsi_update_bus_state(unsigned int out, unsigned int *pirq);

void scsi_update(unsigned short *pdata, unsigned int out, unsigned int *pin, unsigned int *pirq);









#include <linux/proc_fs.h>      /* proc fs */
#include <asm/uaccess.h>        /* copy_from_user */
#include <linux/time.h>
#include <linux/kernel.h>
#include "aotom_main.h"

/*
/proc/stb/fp/aotom_led", "w"
/proc/stb/fp/aotom_icon", "w"
*/
/*
 *  /proc/
 *             |
 *             +--- vfd (w)
 *  /proc/stb/fp
 *             |
 *             +--- aotom (w)
 *             |
 *             +--- version (r)
 *             |
 *             +--- rtc (w)
 *             |
 *             +--- rtc_offset (w)
 *             |
 *             +--- led0_pattern (w)
 *             |
 *             +--- led1_pattern (w)
 *  /proc/stb/lcd
 *             |
 *             +--- scroll_delay (w)
 *             |
 *             +--- show_symbols (w)
 *             |
 *             +--- symbol_network (w)
 *             |
 *             +--- symbol_usb (w)
 *             |
 *             +--- symbol_hdd (w)
 *             |
 *             +--- symbol_hddprogress (w)
 *             |
 *             +--- symbol_signal (w)
 *             |
 *             +--- symbol_timeshift (w)
 *             |
 *             +--- symbol_tv (w)
 *             |
 *             +--- symbol_recording (w)
 *             |
 *             +--- symbol_record_1 (w)
 *             |
 *             +--- symbol_record_2 (w)
 *             |
 *             +--- symbol_smartcard (w)
 *             |
 *             +--- symbol_parent_rating (w)
 *             |
 *             +--- symbol_play (w)
 *             |
 *             +--- oled_brightness (w)
 *  /proc/stb/power/
 *             |
 *             +--- standbyled (w)
*/

extern int install_e2_procs(char *name, read_proc_t *read_proc, write_proc_t *write_proc, void *data);
extern int remove_e2_procs(char *name, read_proc_t *read_proc, write_proc_t *write_proc);

extern int aotomSetIcon(int which, int on);
extern int aotomSetLed(int which, int on);
extern int aotomEnableLed(int which, int on);
extern int aotomWriteText(char *buf, size_t len);
extern int aotomSetBrightness(int level);
extern int aotomGetVersion();

static int rtc_offset = 0;

static int vfd_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  int value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      ret = aotomWriteText(page, count);
    }
    free_page((unsigned long)page);
  }
  return ret;
}

// Proc for accessing quick control of aotom
// String format: fxy
// f is "l" (for led) or "i" (for icons)
// x is 0/1 and indicates if the led/icon must be off or on
// y is the led/icon number (between 0 and LASTLED-1 for leds; between 1 and 46, for icons, with y==46, all the icons are set)
static int aotom_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  int value;
  int ret = -ENOMEM;

  printk("%s(%ld, ", __FUNCTION__, count);

  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      printk("%s", page);

      if (count>3) {
        value = (int)simple_strtol(page+2, NULL, 10);
        if (page[0] == 'l') {
          if (page[1] == '0') {
            if ((value>=0)&&(value<LASTLED)) aotomSetLed(value,0);
          }
          else if (page[1] == '1') {
            if ((value>=0)&&(value<LASTLED)) aotomSetLed(value,1);
          }
        }
        else if (page[0] == 'i') {
          if (page[1] == '0') {
            if ((value>=1)&&(value<=46)) aotomSetIcon(value,0);
          }
          else if (page[1] == '1') {
            if ((value>=1)&&(value<=46)) aotomSetIcon(value,1);
          }
        }
      }

      ret = count;
    }
    free_page((unsigned long)page);
  }
  printk(")\n");

  return ret;
}

static int aotom_read_rtc(char *page, char **start, off_t off, int count, int *eof, void *data) {
	int len = 0;
	u32 rtc_time = YWPANEL_FP_GetTime();
	if(NULL != page)
	{
		/* AOTOM needs time in local time so deduct rtc_offset for e2 */
		len = sprintf(page,"%u\n", rtc_time-rtc_offset);
	}
	return len;
}

static int aotom_write_rtc(struct file *file, const char __user *buffer, unsigned long count, void *data) {
	char *page = NULL;
	ssize_t ret = -ENOMEM;
	u32 argument = 0;
	int test = -1;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
	printk("%s %ld - ", __FUNCTION__, count);
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buffer, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
		printk("%s -> %s\n",__FUNCTION__, myString);
		test = sscanf (myString,"%u",&argument);
		if(0 < test)
		{
			/* AOTOM needs time in local time so add rtc_offset for time from e2 */
			YWPANEL_FP_SetTime(argument+rtc_offset);
			YWPANEL_FP_ControlTimer(true);
		}
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	return ret;
}

static int aotom_read_rtc_offset(char *page, char **start, off_t off, int count, int *eof, void *data) {
	int len = 0;
	if(NULL != page)
		len = sprintf(page,"%d\n", rtc_offset);
	return len;
}

static int aotom_write_rtc_offset(struct file *file, const char __user *buffer, unsigned long count, void *data) {
	char *page = NULL;
	ssize_t ret = -ENOMEM;
	int test = -1;
	char *myString = kmalloc(count + 1, GFP_KERNEL);
	printk("%s %ld - ", __FUNCTION__, count);
	page = (char *)__get_free_page(GFP_KERNEL);
	if (page)
	{
		ret = -EFAULT;
		if (copy_from_user(page, buffer, count))
			goto out;
		strncpy(myString, page, count);
		myString[count] = '\0';
		printk("%s -> %s\n",__FUNCTION__, myString);
		test = sscanf (myString,"%d",&rtc_offset);
		printk(" offset: %d\n",rtc_offset);
		/* always return count to avoid endless loop */
		ret = count;
	}
out:
	free_page((unsigned long)page);
	kfree(myString);
	return ret;
}

static int fp_version_read(char *page, char **start, off_t off, int count,
                          int *eof, void *data_unused)
{
  int len = 0;
  len = sprintf(page, "%d\n", aotomGetVersion());
  return len;
}

static int lcd_symbol_usb_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(13,0);
      else if (value == 1) aotomSetIcon(13,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_symbol_hdd_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(30,0);
      else if (value == 1) aotomSetIcon(30,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_symbol_hddprogress_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(22,0);
      else if (value == 1) aotomSetIcon(22,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_symbol_signal_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(42,0);
      else if (value == 1) aotomSetIcon(42,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_symbol_timeshift_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(43,0);
      else if (value == 1) aotomSetIcon(43,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_symbol_tv_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(27,0);
      else if (value == 1) aotomSetIcon(27,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_symbol_record_1_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(7,0);
      else if (value == 1) aotomSetIcon(7,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_symbol_record_2_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(15,0);
      else if (value == 1) aotomSetIcon(15,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_symbol_smartcard_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(11,0);
      else if (value == 1) aotomSetIcon(11,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_symbol_play_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 0);
      if (value == 0) aotomSetIcon(3,0);
      else if (value == 1) aotomSetIcon(3,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int led0_pattern_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 16);
      if (value==0) aotomSetLed(0,0);
      else if (value==0xffffffff) aotomSetLed(0,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int led1_pattern_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long value;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      value = simple_strtol(page, NULL, 16);
      if (value==0) aotomSetLed(1,0);
      else if (value==0xffffffff) aotomSetLed(1,1);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int lcd_oled_brightness_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  long level;
  int ret = -ENOMEM;
  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';
      level = simple_strtol(page, NULL, 10);
      aotomSetBrightness((int)level);
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int power_standbyled_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  char *page;
  int ret = -ENOMEM;

  printk("%s(%ld, ", __FUNCTION__, count);

  page = (char *)__get_free_page(GFP_KERNEL);
  if (page)
  {
    ret = -EFAULT;
    if (copy_from_user(page, buf, count) == 0)
    {
      page[count] = '\0';

      printk("%s", page);

      if (strcmp(page,"on")==0) {
        aotomEnableLed(0,1);
        printk("on");
      }
      else if (strcmp(page,"off")==0) {
        aotomEnableLed(0,0);
        printk("off");
      }
      ret = count;
    }
    free_page((unsigned long)page);
  }
  return ret;
}

static int null_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
  return count;
}

struct fp_procs
{
  char *name;
  read_proc_t *read_proc;
  write_proc_t *write_proc;
} fp_procs[] =
{
  { "vfd", NULL, vfd_write },
  { "stb/fp/rtc", aotom_read_rtc, aotom_write_rtc },
  { "stb/fp/rtc_offset", aotom_read_rtc_offset, aotom_write_rtc_offset },
  { "stb/fp/aotom", NULL, aotom_write },
  { "stb/fp/led0_pattern", NULL, led0_pattern_write },
  { "stb/fp/led1_pattern", NULL, led1_pattern_write },
  { "stb/fp/version", fp_version_read, NULL },
  { "stb/power/standbyled", NULL, power_standbyled_write },
  { "stb/lcd/scroll_delay", NULL, null_write },
  { "stb/lcd/show_symbols", NULL, null_write },
  { "stb/lcd/symbol_network", NULL, null_write },
  { "stb/lcd/symbol_usb", NULL, lcd_symbol_usb_write },
  { "stb/lcd/symbol_hdd", NULL, lcd_symbol_hdd_write },
  { "stb/lcd/symbol_hddprogress", NULL, lcd_symbol_hddprogress_write },
  { "stb/lcd/symbol_signal", NULL, lcd_symbol_signal_write },
  { "stb/lcd/symbol_timeshift", NULL, lcd_symbol_timeshift_write },
  { "stb/lcd/symbol_tv", NULL, lcd_symbol_tv_write },
  { "stb/lcd/symbol_recording", NULL, null_write },
  { "stb/lcd/symbol_record_1", NULL, lcd_symbol_record_1_write },
  { "stb/lcd/symbol_record_2", NULL, lcd_symbol_record_2_write },
  { "stb/lcd/symbol_smartcard", NULL, lcd_symbol_smartcard_write },
  { "stb/lcd/symbol_parent_rating", NULL, null_write },
  { "stb/lcd/symbol_play", NULL, lcd_symbol_play_write },
  { "stb/lcd/oled_brightness", NULL, lcd_oled_brightness_write },
};

void create_proc_fp(void)
{
  int i;
  for(i = 0; i < sizeof(fp_procs)/sizeof(fp_procs[0]); i++)
  {
    install_e2_procs(fp_procs[i].name, fp_procs[i].read_proc, fp_procs[i].write_proc, NULL);
  }
}

void remove_proc_fp(void)
{
  int i;
  for(i = sizeof(fp_procs)/sizeof(fp_procs[0]) - 1; i >= 0; i--)
  {
    remove_e2_procs(fp_procs[i].name, fp_procs[i].read_proc, fp_procs[i].write_proc);
  }
}


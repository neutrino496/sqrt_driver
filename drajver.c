#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/io.h>               //iowrite ioread
#include <linux/slab.h>             //kmalloc kfree
#include <linux/platform_device.h>  //platform driver
#include <linux/ioport.h>           //ioremap


MODULE_LICENSE("Dual BSD/GPL");
#define BUFF_SIZE 1000
#define DRIVER_NAME "sqrt"
#define LIMIT 32768

#define REG_X_OFFSET     0x0
#define REG_START_OFFSET 0x4
#define REG_Y_OFFSET     0x8
#define REG_READY_OFFSET 0xc


/* adrese jezgara

    #define led_gpio         0x41200000

    #define core_0_reg_x     0x43c00000
    #define core_0_reg_start 0x43c00004
    #define core_0_reg_y     0x43c00008
    #define core_0_reg_ready 0x43c0000c
    #define core_0_reg_end   0x43c0ffff

    #define core_1_reg_x     0x43c10000
    #define core_1_reg_start 0x43c10004
    #define core_1_reg_y     0x43c10008
    #define core_1_reg_ready 0x43c1000c
    #define core_1_reg_end   0x43c1ffff

    #define core_2_reg_x     0x43c20000
    #define core_2_reg_start 0x43c20004
    #define core_2_reg_y     0x43c20008
    #define core_2_reg_ready 0x43c2000c
    #define core_2_reg_end   0x43c2ffff

    #define core_3_reg_x     0x43c30000
    #define core_3_reg_start 0x43c30004
    #define core_3_reg_y     0x43c30008
    #define core_3_reg_ready 0x43c3000c
    #define core_3_reg_end   0x43c3ffff
*/

struct sqrt_info {
  unsigned long mem_start;  // Pocetne fizicke adrese 
  unsigned long mem_end;    // Krajnje fizicke adrese 
  void __iomem *base_addr;  // Pocetne virtuelne adrese 
  
};

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;
static struct sqrt_info *bp0 = NULL;
static struct sqrt_info *bp1 = NULL;
static struct sqrt_info *bp2 = NULL;
static struct sqrt_info *bp3 = NULL;
static struct sqrt_info *bp4 = NULL;

int endRead = 0;
unsigned int core_0=0, core_1=0, core_2=0, core_3=0;
unsigned int i=0, j=0, k=0; 
volatile uint32_t t;
char led[4];
u32 led_val = 0x00;
int numbers[1000];
int korijeni[1000];

static int sqrt_probe(struct platform_device *pdev);
static int sqrt_remove(struct platform_device *pdev);
int sqrt_open(struct inode *pinode, struct file *pfile);
int sqrt_close(struct inode *pinode, struct file *pfile);
ssize_t sqrt_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t sqrt_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);
static int __init sqrt_init(void);
static void __exit sqrt_exit(void);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = sqrt_open,
	.read = sqrt_read,
	.write = sqrt_write,
	.release = sqrt_close,
};

static struct of_device_id sqrt_of_match[] = {
  { .compatible = "led_gpio", },
  { .compatible = "mysqrtip_0", },
  { .compatible = "mysqrtip_1", },
  { .compatible = "mysqrtip_2", },
  { .compatible = "mysqrtip_3", },
  { /* end of list */ },
};

static struct platform_driver sqrt_driver = {
  .driver = {
    .name = DRIVER_NAME,
    .owner = THIS_MODULE,
    .of_match_table	= sqrt_of_match,
  },
  .probe		= sqrt_probe,
  .remove		= sqrt_remove,
};

MODULE_DEVICE_TABLE(of, sqrt_of_match);


static int sqrt_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct resource *r_mem;
    int rc = 0;

    r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!r_mem) {
        printk(KERN_ALERT "Fail sqrt to get resource\n");
        return -ENODEV;
    }

    if (of_device_is_compatible(dev->of_node, "led_gpio")) {
        bp4 = (struct sqrt_info *)kmalloc(sizeof(struct sqrt_info), GFP_KERNEL);
        if (!bp4) {
            printk(KERN_ALERT "Could not allocate sqrt device\n");
            return -ENOMEM;
        }
        bp4->mem_start = 0x41200000;
        bp4->mem_end = 0x41200005;
        if (!request_mem_region(bp4->mem_start, bp4->mem_end - bp4->mem_start + 1, DRIVER_NAME)) {
            printk(KERN_ALERT "Could not lock memory region at %p\n", (void *)bp4->mem_start);
            rc = -EBUSY;
            goto error1;
        }
        bp4->base_addr = ioremap(bp4->mem_start, bp4->mem_end - bp4->mem_start + 1);
        if (!bp4->base_addr) {
            printk(KERN_ALERT "Could not allocate memory\n");
            rc = -EIO;
            goto error6;
        }
        printk(KERN_WARNING "led platform driver registered\n");
    } else if (of_device_is_compatible(dev->of_node, "mysqrtip_0")) {
        bp0 = (struct sqrt_info *)kmalloc(sizeof(struct sqrt_info), GFP_KERNEL);
        if (!bp0) {
            printk(KERN_ALERT "Could not allocate sqrt device\n");
            return -ENOMEM;
        }
        bp0->mem_start = r_mem->start;
        bp0->mem_end = r_mem->end ;
        if (!request_mem_region(bp0->mem_start, bp0->mem_end - bp0->mem_start + 1, DRIVER_NAME)) {
            printk(KERN_ALERT "Could not lock memory region at %p\n", (void *)bp0->mem_start);
            rc = -EBUSY;
            goto error1;
        }
        bp0->base_addr = ioremap(bp0->mem_start, bp0->mem_end - bp0->mem_start + 1);
        if (!bp0->base_addr) {
            printk(KERN_ALERT "Could not allocate memory\n");
            rc = -EIO;
            goto error2;
        }
        printk(KERN_WARNING "sqrt platform driver 0 registered\n");
    } else if (of_device_is_compatible(dev->of_node, "mysqrtip_1")) {
        bp1 = (struct sqrt_info *)kmalloc(sizeof(struct sqrt_info), GFP_KERNEL);
        if (!bp1) {
            printk(KERN_ALERT "Could not allocate sqrt device\n");
            return -ENOMEM;
        }
        bp1->mem_start = r_mem->start;
        bp1->mem_end = r_mem->end;
        if (!request_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start + 1, DRIVER_NAME)) {
            printk(KERN_ALERT "Could not lock memory region at %p\n", (void *)bp1->mem_start);
            rc = -EBUSY;
            goto error1;
        }
        bp1->base_addr = ioremap(bp1->mem_start, bp1->mem_end - bp1->mem_start + 1);
        if (!bp1->base_addr) {
            printk(KERN_ALERT "Could not allocate memory\n");
            rc = -EIO;
            goto error3;
        }
        printk(KERN_WARNING "sqrt platform driver 1 registered\n");
    } else if (of_device_is_compatible(dev->of_node, "mysqrtip_2")) {
        bp2 = (struct sqrt_info *)kmalloc(sizeof(struct sqrt_info), GFP_KERNEL);
        if (!bp2) {
            printk(KERN_ALERT "Could not allocate sqrt device\n");
            return -ENOMEM;
        }
        bp2->mem_start = r_mem->start;
        bp2->mem_end = r_mem->end;
        if (!request_mem_region(bp2->mem_start, bp2->mem_end - bp2->mem_start + 1, DRIVER_NAME)) {
            printk(KERN_ALERT "Could not lock memory region at %p\n", (void *)bp2->mem_start);
            rc = -EBUSY;
            goto error1;
        }
        bp2->base_addr = ioremap(bp2->mem_start, bp2->mem_end - bp2->mem_start + 1);
        if (!bp2->base_addr) {
            printk(KERN_ALERT "Could not allocate memory\n");
            rc = -EIO;
            goto error4;
        }
        printk(KERN_WARNING "sqrt platform driver 2 registered\n");
    } else if (of_device_is_compatible(dev->of_node, "mysqrtip_3")) {
        bp3 = (struct sqrt_info *)kmalloc(sizeof(struct sqrt_info), GFP_KERNEL);
        if (!bp3) {
            printk(KERN_ALERT "Could not allocate sqrt device\n");
            return -ENOMEM;
        }
        bp3->mem_start = r_mem->start;
        bp3->mem_end = r_mem->end;
        if (!request_mem_region(bp3->mem_start, bp3->mem_end - bp3->mem_start + 1, DRIVER_NAME)) {
            printk(KERN_ALERT "Could not lock memory region at %p\n", (void *)bp3->mem_start);
            rc = -EBUSY;
            goto error1;
        }
        bp3->base_addr = ioremap(bp3->mem_start, bp3->mem_end - bp3->mem_start + 1);
        if (!bp3->base_addr) {
            printk(KERN_ALERT "Could not allocate memory\n");
            rc = -EIO;
            goto error5;
        }
        printk(KERN_WARNING "sqrt platform driver 3 registered\n");
    }  else {
        dev_err(dev, "Unsupported device\n");
        return -ENODEV;
    }

    return 0; // ALL OK

    error6:
        release_mem_region(bp4->mem_start, bp4->mem_end - bp4->mem_start + 1);
    error5:
        release_mem_region(bp3->mem_start, bp3->mem_end - bp3->mem_start + 1);
    error4:
        release_mem_region(bp2->mem_start, bp2->mem_end - bp2->mem_start + 1);
    error3:
        release_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start + 1);
    error2:
        release_mem_region(bp0->mem_start, bp0->mem_end - bp0->mem_start + 1);
    error1:
    kfree(bp4);
    kfree(bp3);
    kfree(bp2);
    kfree(bp1);
    kfree(bp0);
    return rc;
}
static int sqrt_remove(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;

    if (of_device_is_compatible(dev->of_node, "mysqrtip_0")) {
        printk(KERN_WARNING "sqrt platform driver removed\n");
        iowrite32(0, bp0->base_addr);
        iounmap(bp0->base_addr);
        release_mem_region(bp0->mem_start, bp0->mem_end - bp0->mem_start + 1);
        kfree(bp0);
    }
    else if (of_device_is_compatible(dev->of_node, "mysqrtip_1")) {
        printk(KERN_WARNING "sqrt platform driver removed\n");
        iowrite32(0, bp1->base_addr);
        iounmap(bp1->base_addr);
        release_mem_region(bp1->mem_start, bp1->mem_end - bp1->mem_start + 1);
        kfree(bp1);
    }
    else if (of_device_is_compatible(dev->of_node, "mysqrtip_2")) {
        printk(KERN_WARNING "sqrt platform driver removed\n");
        iowrite32(0, bp2->base_addr);
        iounmap(bp2->base_addr);
        release_mem_region(bp2->mem_start, bp2->mem_end - bp2->mem_start + 1);
        kfree(bp2);
    }
    else if (of_device_is_compatible(dev->of_node, "mysqrtip_3")) {
        printk(KERN_WARNING "sqrt platform driver removed\n");
        iowrite32(0, bp3->base_addr);
        iounmap(bp3->base_addr);
        release_mem_region(bp3->mem_start, bp3->mem_end - bp3->mem_start + 1);
        kfree(bp3);
    }
    else if (of_device_is_compatible(dev->of_node, "led_gpio")) {
        printk(KERN_WARNING "led platform driver removed\n");
        iowrite32(0, bp4->base_addr);
        iounmap(bp4->base_addr);
        release_mem_region(bp4->mem_start, bp4->mem_end - bp4->mem_start + 1);
        kfree(bp4);
    }

    return 0;
}

int sqrt_open(struct inode *pinode, struct file *pfile){
    printk(KERN_INFO "Succesfully opened sqrt\n");
    return 0;
}
int sqrt_close(struct inode *pinode, struct file *pfile){
    printk(KERN_INFO "Succesfully closed sqrt\n");
    return 0;
}


static void obrada(void) {

    ucitavanje_brojeva:
    if (j%4 == 0 || j > 4) {  // j je koliko je brojeva uneseno 
        iowrite32((u32)numbers[k],   bp0->base_addr + REG_X_OFFSET);
        iowrite32((u32)numbers[k+1], bp1->base_addr + REG_X_OFFSET);
        iowrite32((u32)numbers[k+2], bp2->base_addr + REG_X_OFFSET);
        iowrite32((u32)numbers[k+3], bp3->base_addr + REG_X_OFFSET);

        if (core_0 == 0){
            iowrite32((u32)1, bp0->base_addr + REG_START_OFFSET); // pokretanje prvog jezgra
            iowrite32((u32)0, bp0->base_addr + REG_START_OFFSET);
            core_0 = 1;                 // ovo znaci da je prvo jezgro aktivno
            led_val = led_val | 0x08;   // paljenje prve diode koja simulira rad prvog jezgra
            iowrite32(led_val, bp4->base_addr);
        }

        if (core_1 == 0){
            iowrite32((u32)1, bp1->base_addr + REG_START_OFFSET);
            iowrite32((u32)0, bp1->base_addr + REG_START_OFFSET);
            core_1 = 1; 
            led_val = led_val | 0x04;
            iowrite32(led_val, bp4->base_addr);
        }

        if (core_2 == 0){
            iowrite32((u32)1, bp2->base_addr + REG_START_OFFSET);
            iowrite32((u32)0, bp2->base_addr + REG_START_OFFSET);
            core_2 = 1; 
            led_val = led_val | 0x02;
            iowrite32(led_val, bp4->base_addr);
        }

        if (core_3 == 0){
            iowrite32((u32)1, bp3->base_addr + REG_START_OFFSET);
            iowrite32((u32)0, bp3->base_addr + REG_START_OFFSET);
            core_3 = 1; 
            led_val = led_val | 0x01;
            iowrite32(led_val, bp4->base_addr);
        }


        wait1:
        if(ioread32(bp0->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k] = ioread32(bp0->base_addr + REG_Y_OFFSET);
            core_0 = 0;                 // prvo jezgro prestalo s radom
            led_val = led_val & 0x07;   // 0111 - gasenje prve diode
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait1; }
 
        wait2:
        if(ioread32(bp1->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k+1] = ioread32(bp1->base_addr + REG_Y_OFFSET);
            core_1 = 0; 
            led_val = led_val & 0x0b;  // 1011
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait2; }
        
        wait3:
        if(ioread32(bp2->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k+2] = ioread32(bp2->base_addr + REG_Y_OFFSET);
            core_2 = 0; 
            led_val = led_val & 0x0d;  // 1101
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait3; }

        wait4:
        if(ioread32(bp3->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k+3] = ioread32(bp3->base_addr + REG_Y_OFFSET);
            core_3 = 0; 
            led_val = led_val & 0x0e;  // 1110
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait4; }

        k += 4;  // pomocna promjenjiva koja govori koliko je brojeva do tog momenta uneseno u X registar, tj koliko je korijenova izracunato
        j -= 4;  // govori koliko je brojeva jos ostalo da se obradi
    }
    else if (j%4 == 3 && j < 4) {  
        iowrite32((u32)numbers[k],   bp0->base_addr + REG_X_OFFSET);  
        iowrite32((u32)numbers[k+1], bp1->base_addr + REG_X_OFFSET);
        iowrite32((u32)numbers[k+2], bp2->base_addr + REG_X_OFFSET);

        if (core_0 == 0){  // ako je prvo jezgro neaktivno
            iowrite32((u32)1, bp0->base_addr + REG_START_OFFSET); // pokretanje prvog jezgra
            iowrite32((u32)0, bp0->base_addr + REG_START_OFFSET);
            core_0 = 1; 
            led_val = led_val | 0x08;   // paljenje prve diode koja simulira rad prvog jezgra
            iowrite32(led_val, bp4->base_addr);
        }

        if (core_1 == 0){
            iowrite32((u32)1, bp1->base_addr + REG_START_OFFSET);
            iowrite32((u32)0, bp1->base_addr + REG_START_OFFSET);
            core_1 = 1; 
            led_val = led_val | 0x04;
            iowrite32(led_val, bp4->base_addr);
        }

        if (core_2 == 0){
            iowrite32((u32)1, bp2->base_addr + REG_START_OFFSET);
            iowrite32((u32)0, bp2->base_addr + REG_START_OFFSET);
            core_2 = 1; 
            led_val = led_val | 0x02;
            iowrite32(led_val, bp4->base_addr);
        }

        wait5:
        if(ioread32(bp0->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k] = ioread32(bp0->base_addr + REG_Y_OFFSET);
            core_0 = 0; 
            led_val = led_val & 0x07;  // 0111 - gasenje prve diode
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait5; }

        wait6:
        if(ioread32(bp1->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k+1] = ioread32(bp1->base_addr + REG_Y_OFFSET);
            core_1 = 0; 
            led_val = led_val & 0x0b;  // 1011
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait6; }

        wait7:
        if(ioread32(bp2->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k+2] = ioread32(bp2->base_addr + REG_Y_OFFSET);
            core_2 = 0; 
            led_val = led_val & 0x0d;  // 1101
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait7; }

        k += 3;
        j -= 3;
    }
    else if (j%4 == 2 && j < 4) {
        iowrite32((u32)numbers[k],   bp0->base_addr + REG_X_OFFSET);  
        iowrite32((u32)numbers[k+1], bp1->base_addr + REG_X_OFFSET);

        if (core_0 == 0){
            iowrite32((u32)1, bp0->base_addr + REG_START_OFFSET); // pokretanje prvog jezgra
            iowrite32((u32)0, bp0->base_addr + REG_START_OFFSET);
            core_0 = 1; 
            led_val = led_val | 0x08;   // paljenje prve diode koja simulira rad prvog jezgra
            iowrite32(led_val, bp4->base_addr);
        }

        if (core_1 == 0){
            iowrite32((u32)1, bp1->base_addr + REG_START_OFFSET);
            iowrite32((u32)0, bp1->base_addr + REG_START_OFFSET);
            core_1 = 1; 
            led_val = led_val | 0x04;
            iowrite32(led_val, bp4->base_addr);
        }

        wait8:
        if(ioread32(bp0->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k] = ioread32(bp0->base_addr + REG_Y_OFFSET);
            core_0 = 0; 
            led_val = led_val & 0x07;  // 0111 - gasenje prve diode
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait8; }

        wait9:
        if(ioread32(bp1->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k+1] = ioread32(bp1->base_addr + REG_Y_OFFSET);
            core_1 = 0; 
            led_val = led_val & 0x0b;  // 1011
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait9; }

        k += 2;
        j -= 2;
    }
    else if (j%4 == 1 && j < 4) {
        iowrite32((u32)numbers[k],bp0->base_addr + REG_X_OFFSET); 

        if (core_0 == 0){
            iowrite32((u32)1, bp0->base_addr + REG_START_OFFSET);
            iowrite32((u32)0, bp0->base_addr + REG_START_OFFSET);
            core_0 = 1; 
            led_val = led_val | 0x08;   // paljenje prve diode koja simulira rad prvog jezgra
            iowrite32(led_val, bp4->base_addr);
        }

        wait10:
        if(ioread32(bp0->base_addr + REG_READY_OFFSET) == 1) {
            korijeni[k] = ioread32(bp0->base_addr + REG_Y_OFFSET);
            core_0 = 0; 
            led_val = led_val & 0x07;  // 0111 - gasenje prve diode
            iowrite32(led_val, bp4->base_addr);
        } else { goto wait10; }

        k++;
        j -= 1;
    }

    // ako je zavrsen ispis svih korijena
    if (j==0)
        goto end;

    // ako je ostalo jos brojeva za obradu
    goto ucitavanje_brojeva;

    // kraj programa
    end:
    return;
}

// cita registar
ssize_t sqrt_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
    int ret;
    int len = 0;
    int l = 0;

    char buff[BUFF_SIZE];
	if (endRead) {
		endRead = 0;
		return 0;
	}
    
    while(l < k && endRead == 0) {
        len = scnprintf(buff, BUFF_SIZE, "broj %d - %u:%u \n", l, numbers[l], korijeni[l]);
        ret = copy_to_user(buffer, buff, len);
        if(ret)
			return -EFAULT;

        printk(KERN_INFO "broj %d - %u:%u \n", l, numbers[l], korijeni[l]);
		l++;
		if (l == k) {
			endRead = 1;
		}
	}

    return len;
}

ssize_t sqrt_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{
    char buff[BUFF_SIZE];
	int ret = 0;
    int pos=0;
	int num;

	ret = copy_from_user(buff, buffer, length);
	if(ret)
    {
        printk("KERN INFO Unsuccesful attempt, ret=%d",ret);
		return -EFAULT;
    }
	buff[length] = '\0';

    while(sscanf(buff+pos,"%u%n",&numbers[i],&num)==1) {
        printk("numbers[%d]=%u", i, numbers[i]);
		i++;
		pos=pos+num+1;
	}
    j= i-k;

    obrada();

	return length;
}

static int __init sqrt_init(void)
{
   int ret = 0;
   ret = alloc_chrdev_region(&my_dev_id, 0, 1, DRIVER_NAME);
   if (ret) {
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "sqrt_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");

   my_device = device_create(my_class, NULL, my_dev_id, NULL, DRIVER_NAME);
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

    my_cdev = cdev_alloc();
    my_cdev->ops = &my_fops;
    my_cdev->owner = THIS_MODULE;
    ret = cdev_add(my_cdev, my_dev_id, 1);
    if (ret) {
        printk(KERN_ERR "failed to add cdev\n");
        goto fail_2;
    }
    printk(KERN_INFO "cdev added\n");
    printk(KERN_INFO "Hello world\n");

    ret = platform_driver_register(&sqrt_driver);
    if (ret < 0) {
        printk(KERN_ERR "Failed to register sqrt_driver_0: %d\n", ret);
        goto fail_3;
    }

    printk(KERN_INFO "Hello world\n");
    return 0;

    fail_3:
        platform_driver_unregister(&sqrt_driver);
    fail_2:
       device_destroy(my_class, my_dev_id);
    fail_1:
       class_destroy(my_class);
    fail_0:
       unregister_chrdev_region(my_dev_id, 1);

    return -1;
}

static void __exit sqrt_exit(void)
{
   platform_driver_unregister(&sqrt_driver);
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}

module_init(sqrt_init);
module_exit(sqrt_exit);

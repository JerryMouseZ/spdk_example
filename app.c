#include <assert.h>
#include <spdk/bdev.h>
#include <spdk/bdev_module.h>
#include <spdk/bdev_zone.h>
#include <spdk/env.h>
#include <spdk/event.h>
#include <spdk/init.h>
#include <spdk/thread.h>

static char json_file[] = "bdev.json";
struct spdk_bdev_desc *desc = NULL;
struct spdk_io_channel *channel = NULL;
void *buffer;

void myapp_bdev_event_cb(enum spdk_bdev_event_type type, struct spdk_bdev *bdev,
                         void *event_ctx) {}

void read_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
  spdk_bdev_free_io(bdev_io);

  /* Complete the bdev io and close the channel */
  spdk_bdev_free_io(bdev_io);
  spdk_put_io_channel(channel);
  spdk_bdev_close(desc);
  SPDK_NOTICELOG("Stopping app\n");
  spdk_app_stop(success ? 0 : -1);
}

void write_complete(struct spdk_bdev_io *bdev_io, bool success, void *cb_arg) {
  spdk_bdev_free_io(bdev_io);
  assert(success && "bdev write failed");

  int rc = spdk_bdev_read(desc, channel, buffer, 0, 4096, read_complete, NULL);
  assert(rc == 0 && "spdk_bdev read");
}

void myapp(void *args) {
  int rc =
      spdk_bdev_open_ext("Malloc0", true, myapp_bdev_event_cb, NULL, &desc);
  assert(rc == 0 && "open bdev error");

  struct spdk_bdev *bdev = spdk_bdev_desc_get_bdev(desc);
  channel = spdk_bdev_get_io_channel(desc);
  assert(channel != NULL && "get io channel");

  // call io write
  buffer = spdk_dma_zmalloc(4096, 4096, NULL);
  rc = spdk_bdev_write(desc, channel, buffer, 0, 4096, write_complete, NULL);
  assert(rc == 0 && "spdk_bdev write");
}

int main(int argc, char *argv[]) {
  struct spdk_app_opts opts;
  spdk_app_opts_init(&opts, sizeof(opts));
  opts.name = "myapp";
  opts.json_config_file = json_file;

  spdk_app_start(&opts, myapp, NULL);
  spdk_dma_free(buffer);
  spdk_app_fini();
  return 0;
}

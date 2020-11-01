#include "opengd77.hh"

#include "logger.hh"
#include "config.hh"


#define BSIZE 32

static Radio::Features _open_gd77_features =
{
  true,  // hasDigital
  true,  // hasAnalog
  false, // hasGPS
  8,     // maxNameLength
  16,    // maxIntroLineLength

  1024,  // maxChannels
  16,    // maxChannelNameLength

  32,    // maxZones
  16,    // maxZoneNameLength
  32,    // maxChannelsInZone
  false, // hasABZone

  64,    // maxScanlists
  16,    // maxScanlistNameLength
  32,    // maxChannelsInScanlist
  true,  // scanListNeedPriority

  256,   // maxContacts
  16,    // maxContactNameLength

  /// @bug Check.
  64,   // maxGrouplists
  16,   // maxGrouplistNameLength
  16    // maxContactsInGrouplist
};


OpenGD77::OpenGD77(QObject *parent)
  : Radio(parent), _name("Open GD-77"), _dev(nullptr), _config(nullptr)
{
  // pass...
}

const QString &
OpenGD77::name() const {
  return _name;
}

const Radio::Features &
OpenGD77::features() const {
  return _open_gd77_features;
}

const CodePlug &
OpenGD77::codeplug() const {
  return _codeplug;
}

CodePlug &
OpenGD77::codeplug() {
  return _codeplug;
}


bool
OpenGD77::startDownload(Config *config, bool blocking) {
  if (StatusIdle != _task)
    return false;

  _config = config;
  if (! _config)
    return false;

  _dev = new OpenGD77Interface(this);
  if (! _dev->isOpen()) {
    _errorMessage = tr("In %1(), cannot open OpenGD77 device:\n\t%2").arg(__func__).arg(_dev->errorMessage());
    _dev->deleteLater();
    return false;
  }

  _task = StatusDownload;
  _config->reset();

  if (blocking) {
    run();
    return (StatusIdle == _task);
  }

  // start thread for download
  start();
  return true;
}


bool
OpenGD77::startUpload(Config *config, bool blocking, bool update) {
  if (StatusIdle != _task)
    return false;

  if (! (_config = config))
    return false;

  _dev = new OpenGD77Interface(this);
  if (!_dev->isOpen()) {
    _dev->deleteLater();
    return false;
  }

  _task = StatusUpload;
  if (blocking) {
    run();
    return (StatusIdle == _task);
  }

  // start thread for upload
  start();
  return true;
}


void
OpenGD77::run() {
  if (StatusDownload == _task) {
    download();
  } else if (StatusUpload == _task) {
    upload();
  }
}


void
OpenGD77::download() {
  emit downloadStarted();

  if (_codeplug.numImages() != 2) {
    _errorMessage = QString("In %1(), cannot download codeplug:\n\t"
                            " Codeplug does not contain two images.").arg(__func__);
    _task = StatusError;
    _dev->close();
    _dev->deleteLater();
    emit downloadError(this);
    return;
  }

  // Check every segment in the codeplug
  size_t totb = 0;
  for (int image=0; image<_codeplug.numImages(); image++) {
    for (int n=0; n<_codeplug.image(0).numElements(); n++) {
      if (! _codeplug.image(0).element(n).isAligned(BSIZE)) {
        _errorMessage = QString("In %1(), cannot download codeplug:\n\t Codeplug element %2 (addr=%3, size=%4) "
                                "is not aligned with blocksize %5.").arg(__func__)
            .arg(n).arg(_codeplug.image(0).element(n).address())
            .arg(_codeplug.image(0).element(n).data().size()).arg(BSIZE);
        _task = StatusError;
        _dev->close();
        _dev->deleteLater();
        emit downloadError(this);
        return;
      }
      totb += _codeplug.image(0).element(n).data().size()/BSIZE;
    }
  }

  if (! _dev->read_start(0, 0)) {
    _errorMessage = QString("in %1(), cannot start codeplug download:\n\t %2")
        .arg(__func__).arg(_dev->errorMessage());
    _task = StatusError;
    _dev->close();
    _dev->deleteLater();
    emit downloadError(this);
    return;
  }

  // Then download codeplug
  for (int image=0; image<_codeplug.numImages(); image++) {
    uint32_t bank = (0 == image) ? OpenGD77Codeplug::EEPROM : OpenGD77Codeplug::FLASH;

    size_t bcount = 0;
    for (int n=0; n<_codeplug.image(image).numElements(); n++) {
      uint addr = _codeplug.image(image).element(n).address();
      uint size = _codeplug.image(image).element(n).data().size();
      uint b0 = addr/BSIZE, nb = size/BSIZE;

      for (uint b=0; b<nb; b++, bcount++) {
        if (! _dev->read(bank, (b0+b)*BSIZE, _codeplug.data((b0+b)*BSIZE, image), BSIZE)) {
          _errorMessage = QString("In %1(), cannot read block %2:\n\t %3")
              .arg(__func__).arg(b0+b).arg(_dev->errorMessage());
          _task = StatusError;
          _dev->read_finish();
          _dev->close();
          _dev->deleteLater();
          emit downloadError(this);
          return;
        }
        emit downloadProgress(float(bcount*100)/totb);
      }
    }
    _dev->read_finish();
  }

  _dev->reboot();

  _task = StatusIdle;
  _dev->close();
  _dev->deleteLater();

  emit downloadFinished();
  if (_codeplug.decode(_config))
    emit downloadComplete(this, _config);
  else
    emit downloadError(this);
  _config = nullptr;
}


void
OpenGD77::upload() {
  emit uploadStarted();

  _task = StatusIdle;
  _dev->close();
  _dev->deleteLater();

  emit uploadComplete(this);
}

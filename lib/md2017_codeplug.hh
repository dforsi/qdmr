#ifndef MD2017CODEPLUG_HH
#define MD2017CODEPLUG_HH

#include "tyt_codeplug.hh"

/** Device specific implementation of the codeplug for the TyT MD-2017.
 *
 * @ingroup md2017 */
class MD2017Codeplug : public TyTCodeplug
{
  Q_OBJECT

public:
  /** Constructor. */
  explicit MD2017Codeplug(QObject *parent = nullptr);
  /** Destructor. */
  virtual ~MD2017Codeplug();

public:
  void clearTimestamp();
  bool encodeTimestamp();

  void clearGeneralSettings();
  bool encodeGeneralSettings(Config *config, const Flags &flags);
  bool decodeGeneralSettings(Config *config);

  void clearChannels();
  bool encodeChannels(Config *config, const Flags &flags);
  bool createChannels(CodeplugContext &ctx);
  bool linkChannels(CodeplugContext &ctx);

  void clearContacts();
  bool encodeContacts(Config *config, const Flags &flags);
  bool createContacts(CodeplugContext &ctx);

  void clearZones();
  bool encodeZones(Config *config, const Flags &flags);
  bool createZones(CodeplugContext &ctx);
  bool linkZones(CodeplugContext &ctx);

  void clearGroupLists();
  bool encodeGroupLists(Config *config, const Flags &flags);
  bool createGroupLists(CodeplugContext &ctx);
  bool linkGroupLists(CodeplugContext &ctx);

  void clearScanLists();
  bool encodeScanLists(Config *config, const Flags &flags);
  bool createScanLists(CodeplugContext &ctx);
  bool linkScanLists(CodeplugContext &ctx);

  void clearPositioningSystems();
  bool encodePositioningSystems(Config *config, const Flags &flags);
  bool createPositioningSystems(CodeplugContext &ctx);
  bool linkPositioningSystems(CodeplugContext &ctx);

  void clearBootSettings();
};

#endif // MD2017CODEPLUG_HH

/** @defgroup d578uv2 Anytone AT-D578UV III
 * Device specific classes for Anytone AT-D578UV III.
 *
 * \image html d578uv.jpg "AT-D578UV" width=200px
 * \image latex d578uv.jpg "AT-D578UV" width=200px
 *
 * @ingroup anytone */
#ifndef __D578UV2_HH__
#define __D578UV2_HH__

#include "anytone_radio.hh"
#include "anytone_interface.hh"
#include "d878uv2_callsigndb.hh"

/** Implements an interface to Anytone AT-D578UV VHF/UHF 50W DMR (Tier I & II) radios.
 *
 * @ingroup d578uv2 */
class D578UV2: public AnytoneRadio
{
	Q_OBJECT

public:
  /** Do not construct this class directly, rather use @c Radio::detect. */
  explicit D578UV2(AnytoneInterface *device=nullptr, QObject *parent=nullptr);

  const Radio::Features &features() const;

  /** Returns the default radio information. The actual instance may have different properties
   * due to variants of the same radio. */
  static RadioInfo defaultRadioInfo();

protected:
  /** Holds a copy of the specific radio features. */
  Radio::Features _features;
};

#endif // __D878UV_HH__

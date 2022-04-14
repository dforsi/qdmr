#ifndef D578UV2LIMITS_HH
#define D578UV2LIMITS_HH

#include "anytone_limits.hh"

/** Implements the limits for the AnyTone AT-D878UVII.
 * @ingroup d578uv2 */
class D578UV2Limits: public AnytoneLimits
{
  Q_OBJECT

public:
  /** Constructor. */
  D578UV2Limits(const std::initializer_list<std::pair<double, double>> &freqRanges,
                const QString &hardwareRevision, QObject *parent=nullptr);
};

#endif // D578UV2LIMITS_HH

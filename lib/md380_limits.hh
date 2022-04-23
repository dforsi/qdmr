#ifndef MD380LIMITS_HH
#define MD380LIMITS_HH

#include "radiolimits.hh"

/** Implements the radio limits for TyT MD-390 radios.
 * @ingroup md380 */
class MD380Limits: public RadioLimits
{
  Q_OBJECT

public:
  /** Constructor from frequency ranges. */
  MD380Limits(const std::initializer_list<std::pair<double,double>> &freqRanges, QObject *parent=nullptr);
};

#endif // MD380LIMITS_HH

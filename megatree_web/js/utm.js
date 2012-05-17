<!-- modified from http://www.dorcus.co.uk/carabus//jscalculators.html -->

function utm_to_lat_long(pnt)
{
  var mrm = 0;           // MTM ref meridian
  var xtm    = 0;        // 0 for utm, 1 for mtm, 
  var ellips = 22;       // wgs-84
  var zone = 10;         // UTM zone
  var zletter = "W";     // UTM letter
  var easting = pnt[0];  // x-coordinate
  var northing = pnt[1]; // y-coordinate

  ellipsoid = geo_constants(ellips, xtm);

  // checks
  if (zone < 0 || zone > 60)
    {
    alert("Zone number is out of range. Must be bewteen 0 and 60.");
    ++bulkErrors;
    return;
    }
  if (northing < 0 || northing > 10000000)
    {
    alert("Northing value is out of range");
    ++bulkErrors;
    return;
    }
  if (mrm == 0 && (easting < 160000 || easting > 834000) )
    {
    alert("Easting value is out of range");
    ++bulkErrors;
    return;
    }

  var axis = ellipsoid.axis;
  var eccent = ellipsoid.eccentricity;
  var scaleTm = ellipsoid.scaleTm;
  var eastingOrg = ellipsoid.eastingOrg;
  var k0 = scaleTm;

  var e1 = (1 - Math.sqrt(1 - eccent)) / (1 + Math.sqrt(1 - eccent));
  var x = easting - eastingOrg; //remove 500,000 meter offset for longitude
  var y = northing;

  var nhemisphere = 0;
  if (zletter >= "N" || scaleTm == 0.9999)  // MTM in north hemisphere only
    nhemishere = 1;
  else
    y -= 10000000.0; //remove 10,000,000 meter offset used for southern hemisphere

  var longorig = (zone - 1) * 6 - 180 + 3;  //+3 puts origin in middle of zone
  // @dc 180 - (7+76) * 3 - 1.5
  if (scaleTm == 0.9999)
  {
      // longorig = 180 - (zone*1 + 76) * 3 - 1.5;  // without hack, did string concat !!!
      if (mrm == 0)
      {
         zonerResult = gsrugZoner(0., 0., zone);
         longorig = zonerResult.refMeridian;
      }
      else
         longorig = mrm;
  }
  var eccPrimeSquared = (eccent) / (1-eccent);
  var M = y / k0;
  var mu = M / (axis * (1 - eccent / 4 - 3 * eccent * eccent / 64 - 5 * eccent * eccent * eccent / 256));
  var phi1Rad = mu + (3 * e1 / 2 - 27 * e1 * e1 * e1 / 32) * Math.sin(2 * mu) + (21 * e1 * e1 / 16 - 55 * e1 * e1 * e1 * e1 / 32) * Math.sin(4 * mu) + (151 * e1 * e1 * e1 / 96) * Math.sin(6 * mu);
  var phi1 = phi1Rad * rad2deg;
  var N1 = axis / Math.sqrt(1 - eccent * Math.sin(phi1Rad) * Math.sin(phi1Rad));

  var T1 = Math.tan(phi1Rad) * Math.tan(phi1Rad);
  var C1 = eccPrimeSquared * Math.cos(phi1Rad) * Math.cos(phi1Rad);
  var R1 = axis * (1 - eccent) / Math.pow(1-eccent * Math.sin(phi1Rad) * Math.sin(phi1Rad), 1.5);
  var D = x / (N1 * k0);
  var lat = phi1Rad - (N1 * Math.tan(phi1Rad) / R1) * (D * D / 2 - (5 + 3 * T1 + 10 * C1 - 4 * C1 * C1 - 9 * eccPrimeSquared) * D * D * D * D / 24 + (61 + 90 * T1 + 298 * C1 + 45 * T1 * T1 - 252 * eccPrimeSquared - 3 * C1 * C1) * D * D * D * D * D * D / 720);
  lat = lat * rad2deg;
  var lon = (D - (1 + 2 * T1 + C1) * D * D * D / 6 + (5 - 2 * C1 + 28 * T1 - 3 * C1 * C1 + 8 * eccPrimeSquared + 24 * T1 * T1) * D * D * D * D * D / 120) / Math.cos(phi1Rad);
  lon = longorig + lon * rad2deg;

  return [lat, lon];
}



//===================================================================

function geo_constants(ellipsoid, xtm)
{
  // returns ellipsoid values
  ellipsoid_axis = new Array();
  ellipsoid_eccen = new Array();
  ellipsoid_axis[0] = 6377563.396; ellipsoid_eccen[0] = 0.00667054;  //airy
  ellipsoid_axis[1] = 6377340.189; ellipsoid_eccen[1] = 0.00667054;  // mod airy
  ellipsoid_axis[2] = 6378160; ellipsoid_eccen[2] = 0.006694542;  //aust national
  ellipsoid_axis[3] = 6377397.155; ellipsoid_eccen[3] = 0.006674372;  //bessel 1841
  ellipsoid_axis[4] = 6378206.4; ellipsoid_eccen[4] = 0.006768658;  //clarke 1866 == NAD 27 (TBC)
  ellipsoid_axis[5] = 6378249.145; ellipsoid_eccen[5] = 0.006803511;  //clarke 1880
  ellipsoid_axis[6] = 6377276.345; ellipsoid_eccen[6] = 0.00637847;  //everest
  ellipsoid_axis[7] = 6377304.063; ellipsoid_eccen[7] = 0.006637847;  // mod everest
  ellipsoid_axis[8] = 6378166; ellipsoid_eccen[8] = 0.006693422;  //fischer 1960
  ellipsoid_axis[9] = 6378150; ellipsoid_eccen[9] = 0.006693422;  //fischer 1968
  ellipsoid_axis[10] = 6378155; ellipsoid_eccen[10] = 0.006693422;  // mod fischer
  ellipsoid_axis[11] = 6378160; ellipsoid_eccen[11] = 0.006694605;  //grs 1967
  ellipsoid_axis[12] = 6378137; ellipsoid_eccen[12] = 0.00669438;  //  grs 1980
  ellipsoid_axis[13] = 6378200; ellipsoid_eccen[13] = 0.006693422;  // helmert 1906
  ellipsoid_axis[14] = 6378270; ellipsoid_eccen[14] = 0.006693422;  // hough
  ellipsoid_axis[15] = 6378388; ellipsoid_eccen[15] = 0.00672267;  // int24
  ellipsoid_axis[16] = 6378245; ellipsoid_eccen[16] = 0.006693422;  // krassovsky
  ellipsoid_axis[17] = 6378160; ellipsoid_eccen[17] = 0.006694542;  // s america
  ellipsoid_axis[18] = 6378165; ellipsoid_eccen[18] = 0.006693422;  // wgs-60
  ellipsoid_axis[19] = 6378145; ellipsoid_eccen[19] = 0.006694542;  // wgs-66
  ellipsoid_axis[20] = 6378135; ellipsoid_eccen[20] = 0.006694318;  // wgs-72
  ellipsoid_axis[21] = 6378137; ellipsoid_eccen[21] = 0.00669438;  //wgs-84

  if (ellipsoid == 0)
      ellipsoid = 22;

  --ellipsoid; // table indexed differently
  if (xtm == 1) // MTM
  {
     scaleTm = 0.9999;
     eastingOrg = 304800.;
  }
  else  // UTM
  {
     scaleTm = 0.9996;
     eastingOrg = 500000.;
  }

  // return values as an object
  var ellipsoid = { axis: ellipsoid_axis[ellipsoid],
                    eccentricity: ellipsoid_eccen[ellipsoid],
                    eastingOrg: eastingOrg,
                    scaleTm: scaleTm
                    };
  return ellipsoid;
}

var deg2rad = Math.PI / 180;
var rad2deg = 180.0 / Math.PI;



#include "bmw512-gate.h"

#ifdef BMW512_4WAY

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//#include "sph_keccak.h"
#include "bmw-hash-4way.h"

void bmw512hash_4way(void *state, const void *input)
{
    bmw512_4way_context ctx;
    bmw512_4way_init( &ctx );
    bmw512_4way( &ctx, input, 80 );
    bmw512_4way_close( &ctx, state );
}

int scanhash_bmw512_4way( struct work *work, uint32_t max_nonce,
                          uint64_t *hashes_done, struct thr_info *mythr )
{
   uint32_t vdata[24*4] __attribute__ ((aligned (64)));
   uint32_t hash[16*4] __attribute__ ((aligned (32)));
   uint32_t lane_hash[8] __attribute__ ((aligned (32)));
   uint32_t *hash7 = &(hash[25]);   // 3*8+1
   uint32_t *pdata = work->data;
   uint32_t *ptarget = work->target;
   uint32_t n = pdata[19];
   const uint32_t first_nonce = pdata[19];
   __m256i  *noncev = (__m256i*)vdata + 9;   // aligned
//   const uint32_t Htarg = ptarget[7];
    int thr_id = mythr->id;  // thr_id arg is deprecated

   mm256_bswap32_intrlv80_4x64( vdata, pdata );
   do {
       *noncev = mm256_intrlv_blend_32( mm256_bswap_32(
                _mm256_set_epi32( n+3, 0, n+2, 0, n+1, 0, n, 0 ) ), *noncev );
	
      bmw512hash_4way( hash, vdata );

      for ( int lane = 0; lane < 4; lane++ )
      if ( ( ( hash7[ lane<<1 ] & 0xFFFFFF00 ) == 0 ) )
      {
          extr_lane_4x64( lane_hash, hash, lane, 256 );
          if ( fulltest( lane_hash, ptarget ) )
          {
              pdata[19] = n + lane;
              submit_lane_solution( work, lane_hash, mythr, lane );
          }
      }
      n += 4;

   } while ( (n < max_nonce-4) && !work_restart[thr_id].restart);

   *hashes_done = n - first_nonce + 1;
   return 0;
}

#endif

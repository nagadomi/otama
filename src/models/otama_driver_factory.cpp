/*
 * This file is part of otama.
 *
 * Copyright (C) 2012 nagadomi@nurs.or.jp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "otama_config.h"
#include "otama_driver_factory.hpp"
#include "otama_bovw_fixed_driver.hpp"
#include "otama_lmca_fixed_driver.hpp"
#include "otama_lmca_nodb_driver.hpp"
#include "otama_bovw_inverted_index_driver.hpp"
#include "otama_bovw_nodb_driver.hpp"
#include "otama_bovw_sparse_nodb_driver.hpp"
#include "otama_vlad_nodb_driver.hpp"
#include "otama_sboc_nodb_driver.hpp"
#include "otama_sboc_fixed_driver.hpp"
#include "otama_inverted_index_leveldb.hpp"
#include "otama_inverted_index_bucket.hpp"

using namespace otama;

DriverInterface *
otama::createDriver(otama_variant_t *config)
{
	const char *driver_name;
	otama_variant_t *driver = otama_variant_hash_at(config, "driver");
	
	if (!OTAMA_VARIANT_IS_HASH(config) || !OTAMA_VARIANT_IS_HASH(driver)) {
		OTAMA_LOG_ERROR("Invalid config format: `driver' element not found.", 0);
		return NULL;
	}
	if ((driver_name = otama_variant_hash_at3(driver, "name")) == NULL) {
        OTAMA_LOG_ERROR("Invalid config format: `driver.name' is NULL.", 0);
        return NULL;
	}
	
	// standard set
	if (strcmp(driver_name, "sim") == 0) {
		return new BOVWFixedDriver<NV_BOVW_BIT8K, nv_color_sboc_t>(config);
	}
	else if (strcmp(driver_name, "color") == 0) {
		return new SBOCFixedDriver(config);
	}
#if OTAMA_WITH_LEVELDB
	else if (strcmp(driver_name, "id") == 0) {
		return new BOVWInvertedIndexDriver<NV_BOVW_BIT512K, InvertedIndexLevelDB>(config);
	}
#else
	else if (strcmp(driver_name, "id") == 0)
	{
		return new BOVWInvertedIndexDriver<NV_BOVW_BIT512K, InvertedIndexBucket>(config);
	}
#endif
	if (strcmp(driver_name, "sim_nodb") == 0) {
		return new BOVWNoDBDriver<NV_BOVW_BIT8K, nv_color_sboc_t>(config);
	}
	else if (strcmp(driver_name, "color_nodb") == 0) {
		return new SBOCNoDBDriver(config);
	}
	else if (strcmp(driver_name, "id_nodb") == 0) {
		return new BOVWSparseNoDBDriver<NV_BOVW_BIT512K>(config);
	}
	
	// bovw-gradient-histogram 
	else if (strcmp(driver_name, "bovw2k") == 0) {
		return new BOVWFixedDriver<NV_BOVW_BIT2K, nv_bovw_dummy_color_t>(config);
	}
	else if (strcmp(driver_name, "bovw8k") == 0)
	{
		return new BOVWFixedDriver<NV_BOVW_BIT8K, nv_bovw_dummy_color_t>(config);
	}
	else if (strcmp(driver_name, "bovw512k") == 0)
	{
		return new BOVWFixedDriver<NV_BOVW_BIT512K, nv_bovw_dummy_color_t>(config);
	}
	else if (strcmp(driver_name, "bovw2k_nodb") == 0)
	{
		return new BOVWNoDBDriver<NV_BOVW_BIT2K, nv_bovw_dummy_color_t>(config);
	}
	else if (strcmp(driver_name, "bovw8k_nodb") == 0)
	{
		return new BOVWNoDBDriver<NV_BOVW_BIT8K, nv_bovw_dummy_color_t>(config);
	}
	else if (strcmp(driver_name, "bovw512k_nodb") == 0)
	{
		return new BOVWSparseNoDBDriver<NV_BOVW_BIT512K>(config);
	}
	else if (strcmp(driver_name, "bovw2k_boc") == 0) {
		return new BOVWFixedDriver<NV_BOVW_BIT2K, nv_color_boc_t>(config);
	}
	else if (strcmp(driver_name, "bovw8k_boc") == 0)
	{
		return new BOVWFixedDriver<NV_BOVW_BIT8K, nv_color_boc_t>(config);
	}
	else if (strcmp(driver_name, "bovw512k_boc") == 0)
	{
		return new BOVWFixedDriver<NV_BOVW_BIT512K, nv_color_boc_t>(config);
	}
	else if (strcmp(driver_name, "bovw2k_boc_nodb") == 0)
	{
		return new BOVWNoDBDriver<NV_BOVW_BIT2K, nv_color_boc_t>(config);
	}
	else if (strcmp(driver_name, "bovw8k_boc_nodb") == 0)
	{
		return new BOVWNoDBDriver<NV_BOVW_BIT8K, nv_color_boc_t>(config);
	}
	else if (strcmp(driver_name, "bovw512k_boc_nodb") == 0)
	{
		return new BOVWNoDBDriver<NV_BOVW_BIT512K, nv_color_boc_t>(config);
	}
	else if (strcmp(driver_name, "bovw2k_sboc") == 0) {
		return new BOVWFixedDriver<NV_BOVW_BIT2K, nv_color_sboc_t>(config);
	}
	else if (strcmp(driver_name, "bovw8k_sboc") == 0)
	{
		return new BOVWFixedDriver<NV_BOVW_BIT8K, nv_color_sboc_t>(config);
	}
	else if (strcmp(driver_name, "bovw512k_sboc") == 0)
	{
		return new BOVWFixedDriver<NV_BOVW_BIT512K, nv_color_sboc_t>(config);
	}
	else if (strcmp(driver_name, "bovw2k_sboc_nodb") == 0)
	{
		return new BOVWNoDBDriver<NV_BOVW_BIT2K, nv_color_sboc_t>(config);
	}
	else if (strcmp(driver_name, "bovw8k_sboc_nodb") == 0)
	{
		return new BOVWNoDBDriver<NV_BOVW_BIT8K, nv_color_sboc_t>(config);
	}
	else if (strcmp(driver_name, "bovw512k_sboc_nodb") == 0)
	{
		return new BOVWNoDBDriver<NV_BOVW_BIT512K, nv_color_sboc_t>(config);
	}
	else if (strcmp(driver_name, "bovw512k_iv") == 0)
	{
		return new BOVWInvertedIndexDriver<NV_BOVW_BIT512K, InvertedIndexBucket>(config);
	}
#if OTAMA_WITH_LEVELDB
	else if (strcmp(driver_name, "bovw512k_iv_ldb") == 0)
	{
		return new BOVWInvertedIndexDriver<NV_BOVW_BIT512K, InvertedIndexLevelDB>(config);
	}
#endif
	
	// vald
	else if (strcmp(driver_name, "vlad_nodb") == 0)
	{
		return new VLADNoDBDriver<NV_VLAD_512>(config);
	}
	else if (strcmp(driver_name, "vlad128_nodb") == 0)
	{
		return new VLADNoDBDriver<NV_VLAD_128>(config);
	}
	else if (strcmp(driver_name, "vlad512_nodb") == 0)
	{
		return new VLADNoDBDriver<NV_VLAD_512>(config);
	}
	
	// sboc
	else if (strcmp(driver_name, "sboc") == 0)
	{
		return new SBOCFixedDriver(config);
	}
	else if (strcmp(driver_name, "sboc_nodb") == 0)
	{
		return new SBOCNoDBDriver(config);
	}
	// lmca
	else if (strcmp(driver_name, "lmca_vlad") == 0)
	{
		return new LMCAFixedDriver<NV_LMCA_FEATURE_VLAD, nv_lmca_empty_color_t>(config);
	}
	else if (strcmp(driver_name, "lmca_hsv") == 0)
	{
		return new LMCAFixedDriver<NV_LMCA_FEATURE_HSV, nv_lmca_empty_color_t>(config);
	}
	else if (strcmp(driver_name, "lmca_vlad_hsv") == 0)
	{
		return new LMCAFixedDriver<NV_LMCA_FEATURE_VLAD_HSV, nv_lmca_hsv_t>(config);
	}
	else if (strcmp(driver_name, "lmca_vladhsv") == 0)
	{
		return new LMCAFixedDriver<NV_LMCA_FEATURE_VLADHSV, nv_lmca_empty_color_t>(config);
	}
	else if (strcmp(driver_name, "lmca_vlad_colorcode") == 0)
	{
		return new LMCAFixedDriver<NV_LMCA_FEATURE_VLAD_COLORCODE, nv_lmca_colorcode_t>(config);
	}
	
	else if (strcmp(driver_name, "lmca_vlad_nodb") == 0)
	{
		return new LMCANoDBDriver<NV_LMCA_FEATURE_VLAD, nv_lmca_empty_color_t>(config);
	}
	else if (strcmp(driver_name, "lmca_hsv_nodb") == 0)
	{
		return new LMCANoDBDriver<NV_LMCA_FEATURE_HSV, nv_lmca_empty_color_t>(config);
	}
	else if (strcmp(driver_name, "lmca_vlad_hsv_nodb") == 0)
	{
		return new LMCANoDBDriver<NV_LMCA_FEATURE_VLAD_HSV, nv_lmca_hsv_t>(config);
	}
	else if (strcmp(driver_name, "lmca_vladhsv_nodb") == 0)
	{
		return new LMCANoDBDriver<NV_LMCA_FEATURE_VLADHSV, nv_lmca_empty_color_t>(config);
	}
	else if (strcmp(driver_name, "lmca_vlad_colorcode_nodb") == 0)
	{
		return new LMCANoDBDriver<NV_LMCA_FEATURE_VLAD_COLORCODE, nv_lmca_colorcode_t>(config);
	}
	
	OTAMA_LOG_ERROR("Can't locate `%s' driver.", driver_name);
	
	return NULL;
}

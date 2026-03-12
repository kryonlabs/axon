/*
 * AXON Consensus Building
 * Build consensus from multiple minds and detect contradictions
 */

#include <lib9.h>
#include <9p.h>  /* For vlong types */
#include "axon.h"
#include "minds.h"

/*
 * Consensus cluster - groups matching facts from different minds
 */
typedef struct ConsensusCluster {
    char *key;
    MindFact **facts;         /* Facts from different minds */
    int *mind_indices;        /* Which mind produced each fact */
    int nfacts;
    double avg_confidence;
    int mind_agreement;       /* Bitmask of agreeing minds */
} ConsensusCluster;

/*
 * Group mind facts by subject:predicate:object
 */
ConsensusCluster**
cluster_facts(MindResult **results, int nresults, int *nclusters)
{
    ConsensusCluster **clusters;
    int cluster_count;
    int cluster_capacity;
    int i, j, k;

    if(results == nil || nresults == 0) {
        if(nclusters != nil)
            *nclusters = 0;
        return nil;
    }

    cluster_capacity = 64;
    clusters = emalloc9p(cluster_capacity * sizeof(ConsensusCluster*));
    cluster_count = 0;

    /* Process each mind result */
    for(i = 0; i < nresults; i++) {
        MindResult *result = results[i];
        if(result == nil || result->facts == nil)
            continue;

        /* Process each fact from this mind */
        for(j = 0; j < result->nfacts; j++) {
            MindFact *fact = result->facts[j];
            char *key;
            int found;

            if(fact == nil)
                continue;

            key = mind_fact_key(fact);
            if(key == nil)
                continue;

            /* Look for existing cluster */
            found = 0;
            for(k = 0; k < cluster_count; k++) {
                if(clusters[k] != nil && strcmp(clusters[k]->key, key) == 0) {
                    /* Add to existing cluster */
                    clusters[k]->nfacts++;
                    clusters[k]->facts = erealloc9p(clusters[k]->facts,
                        clusters[k]->nfacts * sizeof(MindFact*));
                    clusters[k]->mind_indices = erealloc9p(clusters[k]->mind_indices,
                        clusters[k]->nfacts * sizeof(int));
                    clusters[k]->facts[clusters[k]->nfacts - 1] = fact;
                    clusters[k]->mind_indices[clusters[k]->nfacts - 1] = result->mind_type;
                    clusters[k]->mind_agreement |= (1 << result->mind_type);
                    found = 1;
                    break;
                }
            }

            /* Create new cluster if not found */
            if(!found) {
                /* Expand capacity if needed */
                if(cluster_count >= cluster_capacity) {
                    cluster_capacity *= 2;
                    clusters = erealloc9p(clusters,
                        cluster_capacity * sizeof(ConsensusCluster*));
                }

                clusters[cluster_count] = emalloc9p(sizeof(ConsensusCluster));
                clusters[cluster_count]->key = key;
                clusters[cluster_count]->facts = emalloc9p(sizeof(MindFact*));
                clusters[cluster_count]->facts[0] = fact;
                clusters[cluster_count]->mind_indices = emalloc9p(sizeof(int));
                clusters[cluster_count]->mind_indices[0] = result->mind_type;
                clusters[cluster_count]->nfacts = 1;
                clusters[cluster_count]->mind_agreement = (1 << result->mind_type);
                clusters[cluster_count]->avg_confidence = fact->confidence;
                cluster_count++;
            } else {
                free(key);
            }
        }
    }

    if(nclusters != nil)
        *nclusters = cluster_count;

    return clusters;
}

/*
 * Build consensus facts from clusters
 */
ConsensusFact**
consensus_build(MindResult **results, int nresults, int *nconsensus)
{
    ConsensusCluster **clusters;
    int nclusters;
    ConsensusFact **consensus;
    int i, j;

    if(results == nil || nresults == 0) {
        if(nconsensus != nil)
            *nconsensus = 0;
        return nil;
    }

    /* Group facts by key */
    clusters = cluster_facts(results, nresults, &nclusters);
    if(clusters == nil || nclusters == 0) {
        if(nconsensus != nil)
            *nconsensus = 0;
        return nil;
    }

    /* Build consensus facts from clusters */
    consensus = emalloc9p(nclusters * sizeof(ConsensusFact*));

    for(i = 0; i < nclusters; i++) {
        ConsensusCluster *cluster = clusters[i];
        ConsensusFact *cf;

        if(cluster == nil)
            continue;

        cf = emalloc9p(sizeof(ConsensusFact));

        /* Extract subject, predicate, object from key */
        char *colon1 = strchr(cluster->key, ':');
        char *colon2 = (colon1 != nil) ? strchr(colon1 + 1, ':') : nil;

        if(colon1 != nil && colon2 != nil) {
            cf->subject = emalloc9p(colon1 - cluster->key + 1);
            memcpy(cf->subject, cluster->key, colon1 - cluster->key);
            cf->subject[colon1 - cluster->key] = '\0';

            cf->predicate = emalloc9p(colon2 - colon1);
            memcpy(cf->predicate, colon1 + 1, colon2 - colon1 - 1);
            cf->predicate[colon2 - colon1 - 1] = '\0';

            cf->object = estrdup9p(colon2 + 1);
        } else {
            cf->subject = estrdup9p(cluster->key);
            cf->predicate = estrdup9p("unknown");
            cf->object = estrdup9p("unknown");
        }

        /* Calculate consensus confidence */
        double sum = 0.0;
        for(j = 0; j < cluster->nfacts; j++) {
            sum += cluster->facts[j]->confidence;
        }
        cf->consensus_confidence = sum / cluster->nfacts;

        cf->source_count = cluster->nfacts;
        cf->mind_agreement = cluster->mind_agreement;

        /* Store per-mind confidences */
        cf->mind_confidences = emalloc9p(MIND_MAX * sizeof(double));
        for(j = 0; j < MIND_MAX; j++) {
            cf->mind_confidences[j] = 0.0;
        }
        for(j = 0; j < cluster->nfacts; j++) {
            int mind_idx = cluster->mind_indices[j];
            if(mind_idx >= 0 && mind_idx < MIND_MAX) {
                cf->mind_confidences[mind_idx] = cluster->facts[j]->confidence;
            }
        }

        /* Store evidence */
        cf->nevidence = cluster->nfacts;
        cf->evidence = emalloc9p(cluster->nfacts * sizeof(char*));
        for(j = 0; j < cluster->nfacts; j++) {
            cf->evidence[j] = cluster->facts[j]->evidence != nil ?
                estrdup9p(cluster->facts[j]->evidence) : estrdup9p("no evidence");
        }

        cf->disagreement_flags = 0;
        cf->contradiction_reason = nil;
        cf->last_updated = time(nil);

        consensus[i] = cf;
    }

    /* Free clusters */
    for(i = 0; i < nclusters; i++) {
        if(clusters[i] != nil) {
            free(clusters[i]->key);
            free(clusters[i]->facts);
            free(clusters[i]->mind_indices);
            free(clusters[i]);
        }
    }
    free(clusters);

    if(nconsensus != nil)
        *nconsensus = nclusters;

    return consensus;
}

/*
 * Check for contradictions among consensus facts
 * A contradiction exists when minds have high confidence disagreement
 */
Contradiction**
contradictions_find(MindResult **results, int nresults, int *ncontra)
{
    Contradiction **contradictions;
    ConsensusCluster **clusters;
    int nclusters;
    int contra_count;
    int contra_capacity;
    int i, j;

    if(results == nil || nresults == 0) {
        if(ncontra != nil)
            *ncontra = 0;
        return nil;
    }

    clusters = cluster_facts(results, nresults, &nclusters);
    if(clusters == nil || nclusters == 0) {
        if(ncontra != nil)
            *ncontra = 0;
        return nil;
    }

    contra_capacity = 16;
    contradictions = emalloc9p(contra_capacity * sizeof(Contradiction*));
    contra_count = 0;

    for(i = 0; i < nclusters; i++) {
        ConsensusCluster *cluster = clusters[i];
        double min_conf, max_conf;
        int has_needs_verification;
        int has_high_conf;

        if(cluster == nil || cluster->nfacts < 2)
            continue;

        /* Find min/max confidence */
        min_conf = 1.0;
        max_conf = 0.0;
        has_needs_verification = 0;
        has_high_conf = 0;

        for(j = 0; j < cluster->nfacts; j++) {
            MindFact *fact = cluster->facts[j];
            if(fact->confidence < min_conf)
                min_conf = fact->confidence;
            if(fact->confidence > max_conf)
                max_conf = fact->confidence;
            if(fact->flags & MFLAG_NEEDS_VERIFICATION)
                has_needs_verification = 1;
            if(fact->confidence > 0.7)
                has_high_conf = 1;
        }

        /* Detect contradiction: high confidence gap */
        if((max_conf - min_conf > 0.5) && has_high_conf) {
            Contradiction *con;

            /* Expand capacity if needed */
            if(contra_count >= contra_capacity) {
                contra_capacity *= 2;
                contradictions = erealloc9p(contradictions,
                    contra_capacity * sizeof(Contradiction*));
            }

            con = emalloc9p(sizeof(Contradiction));
            con->fact_key = estrdup9p(cluster->key);
            con->consensus = nil;  /* Will be filled later */
            con->conflicting_claims = emalloc9p(cluster->nfacts * sizeof(MindFact*));
            con->nclaims = cluster->nfacts;
            for(j = 0; j < cluster->nfacts; j++) {
                con->conflicting_claims[j] = cluster->facts[j];
            }
            con->resolution = nil;
            con->resolved = 0;
            con->discovered_at = time(nil);
            con->resolved_at = 0;

            contradictions[contra_count++] = con;
        }
    }

    /* Free clusters */
    for(i = 0; i < nclusters; i++) {
        if(clusters[i] != nil) {
            free(clusters[i]->key);
            free(clusters[i]->facts);
            free(clusters[i]->mind_indices);
            free(clusters[i]);
        }
    }
    free(clusters);

    if(ncontra != nil)
        *ncontra = contra_count;

    return contradictions;
}

/*
 * Resolve a contradiction
 */
int
contradiction_resolve(Contradiction *con, char *resolution)
{
    if(con == nil)
        return -1;

    if(resolution != nil)
        con->resolution = estrdup9p(resolution);

    con->resolved = 1;
    con->resolved_at = time(nil);

    return 0;
}

/*
 * Update consensus with new mind results
 */
int
consensus_update(ConsensusFact *consensus, MindResult **results, int nresults)
{
    /* Placeholder - rebuild consensus */
    if(consensus == nil || results == nil || nresults == 0)
        return -1;

    /* For Phase 1, just update timestamp */
    consensus->last_updated = time(nil);

    return 0;
}

/*
 * Calculate average confidence for a cluster
 */
double
cluster_confidence(ConsensusCluster *cluster)
{
    int i;
    double sum;

    if(cluster == nil || cluster->nfacts == 0)
        return 0.0;

    sum = 0.0;
    for(i = 0; i < cluster->nfacts; i++) {
        sum += cluster->facts[i]->confidence;
    }

    return sum / cluster->nfacts;
}

/*
 * Free consensus fact
 */
void
consensus_fact_free(ConsensusFact *fact)
{
    int i;

    if(fact == nil)
        return;

    free(fact->subject);
    free(fact->predicate);
    free(fact->object);
    free(fact->mind_confidences);

    if(fact->evidence != nil) {
        for(i = 0; i < fact->nevidence; i++) {
            free(fact->evidence[i]);
        }
        free(fact->evidence);
    }

    free(fact->contradiction_reason);
    free(fact);
}

/*
 * Free contradiction
 */
void
contradiction_free(Contradiction *con)
{
    if(con == nil)
        return;

    free(con->fact_key);
    if(con->conflicting_claims != nil)
        free(con->conflicting_claims);  /* Don't free facts themselves */
    if(con->consensus != nil)
        consensus_fact_free(con->consensus);
    free(con->resolution);
    free(con);
}

/*
 * Free cluster structure
 */
void
cluster_free(ConsensusCluster *cluster)
{
    int i;

    if(cluster == nil)
        return;

    free(cluster->key);
    if(cluster->facts != nil) {
        /* Don't free the facts themselves, just the array */
        free(cluster->facts);
    }
    free(cluster->mind_indices);
    free(cluster);
}

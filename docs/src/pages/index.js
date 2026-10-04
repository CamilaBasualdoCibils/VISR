import React from 'react';
import Layout from '@theme/Layout';
import Link from '@docusaurus/Link';
import Heading from '@theme/Heading';
import styles from './index.module.css';

const areas = [
  ['01', 'Design', 'The principles and intended shape of a portable spatial interface.', '/docs/design'],
  ['02', 'Architecture', 'How language, runtime, rendering, and device boundaries fit together.', '/docs/architecture'],
  ['03', 'Concepts', 'A growing vocabulary for understanding VISR.', '/docs/concepts'],
  ['04', 'Hardware', 'Capability boundaries and future device integrations.', '/docs/hardware'],
  ['05', 'Applications', 'The contract between applications and the environment.', '/docs/applications'],
  ['06', 'Internals', 'A guided map of the codebase and its main subsystems.', '/docs/internals'],
];

export default function Home() {
  return <Layout title="A spatial interface runtime" description="VISR project documentation">
    <main className={styles.main}>
      <section className={styles.hero}>
        <div className={styles.eyebrow}><span /> OPEN INTERFACE RUNTIME</div>
        <Heading as="h1">Interfaces that<br /><span>belong in space.</span></Heading>
        <p>VISR is an evolving runtime for portable, capability-aware interfaces across spatial environments.</p>
        <div className={styles.actions}>
          <Link className="button button--primary button--lg" to="/docs/getting-started">Explore VISR <span aria-hidden>↗</span></Link>
          <Link className={styles.textLink} to="/docs/architecture">Read the architecture →</Link>
        </div>
        <div className={styles.orbit} aria-hidden="true"><div className={styles.orbitCore}>V</div><i /><i /><i /></div>
        <div className={styles.caption}>A SYSTEM FOR CONTEXTUAL COMPUTING <span>—</span> 2026</div>
      </section>
      <section className={styles.explore}>
        <div className={styles.sectionHeading}><div><div className={styles.kicker}>THE PROJECT</div><Heading as="h2">Explore the system</Heading></div><p>Start with the ideas, then follow them into the implementation.</p></div>
        <div className={styles.grid}>{areas.map(([number, title, body, to]) => <Link className={styles.card} to={to} key={title}>
          <span className={styles.number}>{number}</span><span className={styles.arrow}>↗</span><Heading as="h3">{title}</Heading><p>{body}</p>
        </Link>)}</div>
      </section>
    </main>
  </Layout>;
}

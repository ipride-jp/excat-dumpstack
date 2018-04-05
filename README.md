<section class="sub-content"><h2 class="sub-title">Excat for Java</h2>

<h3 class="sub-title2">概要</h3>

<p>Javaアプリケーション障害の原因解析を支援する障害情報取得・表示ツールです。</p>

<h3 class="sub-title2">特長</h3>

<ul class="li-square"><li>障害情報取得条件をリアルタイムに変更可能です。<br>
	（対象プログラムのソースコードに手を加える必要はありません）</li>
	<li>イベントトリガー型ですので性能への影響は軽微です。</li>

</ul></section>
<section class="sub-content"><h2 class="sub-title">Excat for Javaの概要</h2>

<ul class="li-gaiyo"><li>指定したトリガー条件（例外、メソッド呼出）の発生ならびに外部シグナルを契機に、スタック情報、メソッドのパラメータ、ローカル変数、インスタンスを瞬時に取得します。</li>
	<li>取得情報を見やすい形で表示するビューアを提供します（ソースコード、もしくはバイトコードとの対応表示も可能）。</li>
</ul><p><a name="excat-koka" id="excat-koka"></a></p>
</section>
<section class="sub-content"><h2 class="sub-title">Excat for Java適用効果</h2>

<table class="product-line-list"><tbody><tr><th>障害解析時の問題点</th>
			<th colspan="2">Excat for Java適用効果</th>
		</tr><tr><td>ログの不足</td>
			<td>監視対象の例外やエラーが発生した時点で、該当スタックトレース及び関連するオブジェクトのスナップショットを取得します。<br>
			メソッドの引数、オブジェクトの属性や変数、コレクションの中身まで取得できるため、「デバッグログが足りない！」という状況にはもうおちいりません。</td>
			<td>⇒&nbsp;障害再現までのコストを大幅に削減！</td>
		</tr><tr><td>環境依存</td>
			<td>データ、設定条件、システム構成、発生タイミングに依存して発生する障害に対し適確な情報取得ができます。</td>
			<td>⇒&nbsp;開発環境での再現を待たずとも、原因特定の可能性が高まります！</td>
		</tr><tr><td>ログを容易に追加できない</td>
			<td>監視対象の設定はいつでも自由に変更できます。ログ出力のコードを追加したモジュールを本番検証環境に再リリースするといった作業は必要ありません。</td>
			<td>⇒&nbsp;APサーバー再起動不要で容易にログの追加が可能！</td>
		</tr><tr><td>余計なログが多い</td>
			<td>監視対象の例外やエラーに関連する情報だけがスナップショットに保存されるので、大量のログの中から障害のログだけを特定して抜き出す作業はもう必要ありません。</td>
			<td>⇒&nbsp;余計なログから解放され、より原因特定が容易に！</td>
		</tr></tbody></table><p><a name="excat-download" id="excat-download"></a></p>
</section>
